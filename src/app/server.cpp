/**
 * Unified Engineering Workbench — HTTP API Server
 * Provides REST endpoints for all five workbench modules.
 * Serves the frontend from ./frontend/
 */

// Do NOT define CPPHTTPLIB_OPENSSL_SUPPORT — that would enable SSL
#include "httplib.h"
#include "thermal.hpp"
#include "importers.hpp"
#include "backprop.hpp"
#include "benchmark.hpp"
#include "embedded.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <filesystem>
#include <iomanip>
#include <algorithm>
#include <thread>
#include <cstdlib>
#include <atomic>

// ─── JSON helpers ───────────────────────────────────────────────────────────

static std::string json_str(const std::string& s) {
    std::string out; out.reserve(s.size() + 2); out += '"';
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\t"; break;
            default:   out += c;
        }
    }
    out += '"'; return out;
}

static std::string json_dbl(double v, int prec = 10) {
    if (!std::isfinite(v)) return "null";
    std::ostringstream ss; ss << std::setprecision(prec) << v;
    return ss.str();
}

// ─── CORS helper ─────────────────────────────────────────────────────────────

static void add_cors(httplib::Response& res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
}

static void json_ok(httplib::Response& res, const std::string& body) {
    add_cors(res);
    res.set_content(body, "application/json");
}

static void json_err(httplib::Response& res, int code, const std::string& msg) {
    add_cors(res);
    res.status = code;
    res.set_content("{\"error\":" + json_str(msg) + "}", "application/json");
}

static std::string make_upload_temp_path(const char* extension) {
    static std::atomic<unsigned long long> sequence{0};
    return "results/_tmp_upload_" + std::to_string(sequence.fetch_add(1)) + extension;
}
struct ScopedTempFile {
    explicit ScopedTempFile(std::string value) : path(std::move(value)) {}
    ~ScopedTempFile() { std::error_code ec; std::filesystem::remove(path, ec); }
    std::string path;
};

// ─── Thermal helpers ─────────────────────────────────────────────────────────

static std::string thermal_result_to_json(const ThermalResult& r,
                                           const ThermalConfig& cfg,
                                           int n) {
    std::ostringstream j;
    j << std::setprecision(12);
    j << "{\"n\":" << n
      << ",\"max_midpoint_error_C\":" << json_dbl(r.max_midpoint_error)
      << ",\"nodes\":[";
    for (std::size_t i = 0; i < r.x.size(); ++i) {
        double xval = r.x[i];
        double fem  = r.temperature[i];
        double exact = exact_temperature(cfg, xval);
        if (i) j << ",";
        j << "{\"x\":" << json_dbl(xval)
          << ",\"T_fem\":" << json_dbl(fem)
          << ",\"T_exact\":" << json_dbl(exact)
          << ",\"error\":" << json_dbl(std::abs(fem - exact)) << "}";
    }
    j << "]}";
    return j.str();
}

// ─── Main ───────────────────────────────────────────────────────────────────

int main() {
    httplib::Server svr;

    // Serve frontend files
    if (!svr.set_mount_point("/", "./frontend")) {
        std::cerr << "Warning: ./frontend directory not found.\n";
    }

    // ── OPTIONS (CORS preflight) ─────────────────────────────────────────────
    svr.Options(".*", [](const httplib::Request&, httplib::Response& res) {
        add_cors(res);
        res.status = 204;
    });

    // ── GET /api/health ──────────────────────────────────────────────────────
    svr.Get("/api/health", [](const httplib::Request&, httplib::Response& res) {
        unsigned hc = std::thread::hardware_concurrency();
        std::ostringstream j;
        j << "{\"status\":\"online\","
          << "\"backend\":\"C++17\","
          << "\"application\":\"Unified Engineering Workbench\","
          << "\"logical_cpus\":" << (hc ? hc : 1) << "}";
        json_ok(res, j.str());
    });

    // ── GET /api/thermal ─────────────────────────────────────────────────────
    svr.Get("/api/thermal", [](const httplib::Request&, httplib::Response& res) {
        try {
            ThermalConfig cfg;
            std::filesystem::create_directories("results");
            std::ofstream convergence("results/thermal_convergence.csv");
            if (!convergence) throw std::runtime_error("Cannot write thermal convergence results.");
            convergence << "elements,max_midpoint_error_C\n" << std::setprecision(12);
            std::ostringstream j;
            j << "{\"config\":{"
              << "\"L\":" << cfg.length
              << ",\"A\":" << cfg.area
              << ",\"k\":" << cfg.conductivity
              << ",\"q\":" << cfg.generation
              << ",\"T0\":" << cfg.left_temperature
              << ",\"TL\":" << cfg.right_temperature
              << "},\"meshes\":[";
            bool first = true;
            for (int n : {10, 20, 40, 80}) {
                auto r = solve_thermal(cfg, n);
                write_thermal_csv(r, "results/thermal_N" + std::to_string(n) + ".csv");
                convergence << n << "," << r.max_midpoint_error << "\n";
                if (!first) j << ",";
                j << thermal_result_to_json(r, cfg, n);
                first = false;
            }
            j << "]}";
            json_ok(res, j.str());
        } catch (const std::exception& e) {
            json_err(res, 500, e.what());
        }
    });

    // ── GET /api/benchmark ───────────────────────────────────────────────────
    svr.Get("/api/benchmark", [](const httplib::Request& req, httplib::Response& res) {
        try {
            std::filesystem::create_directories("results");
            auto parse_int = [&](const char* key, int fallback, int minimum, int maximum) {
                if (!req.has_param(key)) return fallback;
                const auto value = req.get_param_value(key);
                std::size_t used = 0;
                int parsed = std::stoi(value, &used);
                if (used != value.size() || parsed < minimum || parsed > maximum)
                    throw std::invalid_argument(std::string(key) + " must be in range " + std::to_string(minimum) + ".." + std::to_string(maximum));
                return parsed;
            };
            const int case_count = parse_int("cases", 2000, 1, 100000);
            const int mesh_size = parse_int("elements", 200, 2, 10000);
            const unsigned seed = static_cast<unsigned>(parse_int("seed", 2026, 0, 2147483647));
            auto rows = run_benchmark("results/benchmark.csv", case_count, mesh_size, seed);
            std::ostringstream j;
            j << std::setprecision(12);
            j << "{\"rows\":[";
            for (std::size_t i = 0; i < rows.size(); ++i) {
                const auto& r = rows[i];
                if (i) j << ",";
                j << "{"
                  << "\"workers\":" << r.workers
                  << ",\"median_ms\":" << json_dbl(r.median_ms)
                  << ",\"speedup\":" << json_dbl(r.speedup)
                  << ",\"efficiency\":" << json_dbl(r.efficiency)
                  << ",\"max_abs_difference_C\":" << json_dbl(r.max_abs_difference_C)
                  << ",\"cases\":" << r.cases
                  << ",\"elements\":" << r.elements
                  << ",\"repetitions\":" << r.repetitions
                  << ",\"seed\":" << r.seed
                  << ",\"logical_cpus\":" << r.logical_cpus
                  << "}";
            }
            j << "]}";
            json_ok(res, j.str());
        } catch (const std::exception& e) {
            json_err(res, 500, e.what());
        }
    });

    // ── GET /api/backprop ────────────────────────────────────────────────────
    svr.Get("/api/backprop", [](const httplib::Request&, httplib::Response& res) {
        try {
            std::filesystem::create_directories("results");
            write_loss_csv("results/backprop_loss.csv", 100, 0.1);
            // Run one-step result for gradient info
            auto r = run_backprop(1.0, 2.0, 0.5, 0.0, 1.0, 0.0, 0.1, 1e-6);

            // Generate training history (100 steps)
            std::vector<std::pair<int,double>> history;
            double x=1, y=2, w=0.5, b=0, v=1, c=0, lr=0.1;
            for (int s = 0; s <= 100; ++s) {
                double h = std::tanh(w*x+b), yh = v*h+c, e = yh-y;
                history.push_back({s, 0.5*e*e});
                auto step = run_backprop(x, y, w, b, v, c, lr, 1e-6);
                w = step.updated[0]; b = step.updated[1];
                v = step.updated[2]; c = step.updated[3];
            }

            std::ostringstream j;
            j << std::setprecision(12);
            j << "{"
              << "\"x\":1,\"target\":2,"
              << "\"initial_params\":{\"w\":0.5,\"b\":0,\"v\":1,\"c\":0},"
              << "\"learning_rate\":0.1,"
              << "\"first_step\":{"
              << "\"h\":" << json_dbl(r.h)
              << ",\"prediction\":" << json_dbl(r.prediction)
              << ",\"loss\":" << json_dbl(r.loss)
              << ",\"gradients\":{\"dJ_dw\":" << json_dbl(r.gradient[0])
              << ",\"dJ_db\":" << json_dbl(r.gradient[1])
              << ",\"dJ_dv\":" << json_dbl(r.gradient[2])
              << ",\"dJ_dc\":" << json_dbl(r.gradient[3]) << "}"
              << ",\"max_fd_error\":" << json_dbl(r.max_finite_difference_error)
              << "},"
              << "\"history\":[";
            for (std::size_t i = 0; i < history.size(); ++i) {
                if (i) j << ",";
                j << "{\"step\":" << history[i].first
                  << ",\"loss\":" << json_dbl(history[i].second) << "}";
            }
            j << "]}";
            json_ok(res, j.str());
        } catch (const std::exception& e) {
            json_err(res, 500, e.what());
        }
    });

    // ── GET /api/hardware ────────────────────────────────────────────────────
    svr.Get("/api/hardware", [](const httplib::Request& req, httplib::Response& res) {
        try {
            std::string action = req.has_param("action") ? req.get_param_value("action") : "";
            int id = req.has_param("id") ? std::stoi(req.get_param_value("id")) : -1;
            auto devices = update_embedded_device(action, id);

            std::ostringstream j;
            j << "{\"devices\":[";
            for (std::size_t i = 0; i < devices.size(); ++i) {
                const auto& d = devices[i];
                if (i) j << ",";
                j << "{"
                  << "\"id\":" << i
                  << ",\"type\":" << json_str(d.type)
                  << ",\"name\":" << json_str(d.name)
                  << ",\"state\":" << json_str(d.state)
                  << ",\"message\":" << json_str(d.message)
                  << ",\"capabilities\":" << json_str(d.capabilities)
                  << "}";
            }
            j << "]}";
            json_ok(res, j.str());
        } catch (const std::exception& e) {
            json_err(res, 500, e.what());
        }
    });

    // ── POST /api/geometry/stl ───────────────────────────────────────────────
    svr.Post("/api/geometry/stl", [](const httplib::Request& req,
                                      httplib::Response& res) {
        try {
            // Expect multipart or raw body
            std::string content;
            if (req.is_multipart_form_data() && req.form.has_file("file")) {
                content = req.form.get_file("file").content;
            } else {
                content = req.body;
            }
            if (content.empty()) {
                json_err(res, 400, "No file content received.");
                return;
            }
            if (content.size() > 20 * 1024 * 1024) {
                json_err(res, 413, "File exceeds 20 MiB limit.");
                return;
            }

            // Write to temp file for validator
            std::filesystem::create_directories("results");
            ScopedTempFile temp_file(make_upload_temp_path(".stl"));
            const std::string& tmp = temp_file.path;
            {
                std::ofstream tf(tmp, std::ios::binary);
                if (!tf) { json_err(res, 500, "Cannot create temp file."); return; }
                tf << content;
            }

            auto stats = validate_ascii_stl(tmp, 20*1024*1024);

            // Parse vertices for 3D preview
            std::vector<std::array<double,3>> verts;
            std::istringstream ss(content);
            std::string line;
            while (std::getline(ss, line)) {
                // trim
                std::size_t p = line.find("vertex");
                if (p == std::string::npos) continue;
                std::istringstream ls(line.substr(p + 6));
                double x, y, z;
                if (ls >> x >> y >> z) {
                    verts.push_back({x, y, z});
                }
            }

            std::ostringstream j;
            j << std::setprecision(10);
            j << "{"
              << "\"triangles\":" << stats.triangles
              << ",\"bounds\":{"
              << "\"x\":[" << stats.min[0] << "," << stats.max[0] << "]"
              << ",\"y\":[" << stats.min[1] << "," << stats.max[1] << "]"
              << ",\"z\":[" << stats.min[2] << "," << stats.max[2] << "]"
              << "},"
              << "\"vertices\":[";
            for (std::size_t i = 0; i < verts.size(); ++i) {
                if (i) j << ",";
                j << "[" << verts[i][0] << ","
                  << verts[i][1] << ","
                  << verts[i][2] << "]";
            }
            j << "]}";
            json_ok(res, j.str());
        } catch (const std::exception& e) {
            json_err(res, 400, e.what());
        }
    });

    // ── POST /api/geometry/csv ───────────────────────────────────────────────
    svr.Post("/api/geometry/csv", [](const httplib::Request& req,
                                      httplib::Response& res) {
        try {
            std::string content;
            if (req.is_multipart_form_data() && req.form.has_file("file")) {
                content = req.form.get_file("file").content;
            } else {
                content = req.body;
            }
            if (content.empty()) {
                json_err(res, 400, "No file content received.");
                return;
            }

            std::filesystem::create_directories("results");
            ScopedTempFile temp_file(make_upload_temp_path(".csv"));
            const std::string& tmp = temp_file.path;
            {
                std::ofstream tf(tmp);
                if (!tf) { json_err(res, 500, "Cannot create temp file."); return; }
                tf << content;
            }

            auto stats = validate_temperature_csv(tmp, 20*1024*1024);

            // Parse nodes for colour cloud
            std::vector<std::array<double,5>> nodes;
            std::istringstream ss(content);
            std::string line;
            std::getline(ss, line); // skip header
            while (std::getline(ss, line)) {
                if (line.empty()) continue;
                std::istringstream ls(line);
                std::string cell;
                double vals[5]; int ci = 0;
                while (std::getline(ls, cell, ',') && ci < 5) {
                    try { vals[ci++] = std::stod(cell); } catch(...) { break; }
                }
                if (ci == 5) nodes.push_back({vals[0],vals[1],vals[2],vals[3],vals[4]});
            }

            std::ostringstream j;
            j << std::setprecision(10);
            j << "{"
              << "\"nodes_count\":" << stats.nodes
              << ",\"temp_min\":" << json_dbl(stats.min_temp)
              << ",\"temp_max\":" << json_dbl(stats.max_temp)
              << ",\"nodes\":[";
            for (std::size_t i = 0; i < nodes.size(); ++i) {
                if (i) j << ",";
                j << "{\"id\":" << (int)nodes[i][0]
                  << ",\"x\":" << json_dbl(nodes[i][1])
                  << ",\"y\":" << json_dbl(nodes[i][2])
                  << ",\"z\":" << json_dbl(nodes[i][3])
                  << ",\"T\":" << json_dbl(nodes[i][4]) << "}";
            }
            j << "]}";
            json_ok(res, j.str());
        } catch (const std::exception& e) {
            json_err(res, 400, e.what());
        }
    });

    // ── GET /api/results/{name} ──────────────────────────────────────────────
    // Serve result CSV files
    svr.Get("/api/results/([a-zA-Z0-9_.-]+)", [](const httplib::Request& req,
                                                    httplib::Response& res) {
        std::string name = req.matches[1];
        // Security: only allow known extensions
        if (name.find("..") != std::string::npos ||
            (name.size() < 4) ||
            (name.substr(name.size()-4) != ".csv" &&
             name.substr(name.size()-4) != ".png" &&
             name.substr(name.size()-4) != ".svg")) {
            json_err(res, 400, "Invalid file name.");
            return;
        }
        std::string path = "results/" + name;
        std::ifstream f(path, std::ios::binary);
        if (!f) {
            json_err(res, 404, "File not found: " + name);
            return;
        }
        std::ostringstream buf;
        buf << f.rdbuf();
        std::string ct = "text/csv";
        if (name.size() >= 4 && name.substr(name.size()-4) == ".png") ct = "image/png";
        if (name.size() >= 4 && name.substr(name.size()-4) == ".svg") ct = "image/svg+xml";
        add_cors(res);
        res.set_content(buf.str(), ct.c_str());
    });

    int port = 8080;
    if (const char* configured = std::getenv("UEW_PORT")) {
        try { port = std::stoi(configured); } catch (...) { std::cerr << "Invalid UEW_PORT; using 8080.\n"; }
    }
    if (port < 1024 || port > 65535) { std::cerr << "UEW_PORT must be between 1024 and 65535.\n"; return 2; }
    std::cout << "======================================\n";
    std::cout << "Unified Engineering Workbench API\n";
    std::cout << "Server: http://127.0.0.1:" << port << "\n";
    std::cout << "Dashboard: http://127.0.0.1:" << port << "/index.html\n";
    std::cout << "======================================\n";

    if (!svr.listen("127.0.0.1", port)) {
        std::cerr << "Unable to start server on port " << port << ".\n";
        return 1;
    }
    return 0;
}
