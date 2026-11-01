'use strict';

const $ = id => document.getElementById(id);
function scrollToSection(id) { const target=$(id); if(target) target.scrollIntoView({behavior:'smooth',block:'start'}); }
const api = async (path, options = {}) => {
  const response = await fetch(path, options);
  const body = await response.json().catch(() => ({}));
  if (!response.ok) throw new Error(body.error || `HTTP ${response.status}`);
  return body;
};
const setStatus = (id, message, good = true) => {
  const el = $(id); if (!el) return;
  el.textContent = message; el.className = `status-box ${good ? 'ok' : 'warn'}`;
};
const fmt = (n, digits = 6) => Number(n).toFixed(digits);

// Server health is checked on startup; all compute and parsing remain in C++.
(async () => {
  try { await api('/api/health'); $('status-text').textContent = 'C++ backend online'; $('server-status').className = 'status-pill status-online'; }
  catch (e) { $('status-text').textContent = `Backend unavailable: ${e.message}`; $('server-status').className = 'status-pill status-offline'; }
})();

// Canvas-based 3D projection keeps the standalone application offline-capable.
function create3DCanvas(canvas, points, mode) {
  canvas.style.display = 'block'; canvas.width = Math.max(480, canvas.clientWidth || 480); canvas.height = 300;
  const ctx = canvas.getContext('2d');
  let yaw = .65, pitch = -.35, zoom = 1, panX = 0, panY = 0, dragging = false, lastX = 0, lastY = 0, selected = null;
  const xyz = points.map(p => mode === 'mesh' ? p : [p.x, p.y, p.z]);
  const lo = [0,1,2].map(k => xyz.reduce((v,p) => Math.min(v,p[k]), Infinity));
  const hi = [0,1,2].map(k => xyz.reduce((v,p) => Math.max(v,p[k]), -Infinity));
  const center = lo.map((v,k) => (v+hi[k])/2);
  const span = Math.max(...lo.map((v,k) => hi[k]-v), 1e-9);
  const project = p => {
    let x=(p[0]-center[0])/span, y=(p[1]-center[1])/span, z=(p[2]-center[2])/span;
    const xx=x*Math.cos(yaw)-y*Math.sin(yaw), yy=x*Math.sin(yaw)+y*Math.cos(yaw);
    const py=yy*Math.cos(pitch)-z*Math.sin(pitch), depth=yy*Math.sin(pitch)+z*Math.cos(pitch);
    return {x:canvas.width/2+panX+xx*zoom*canvas.width*.72, y:canvas.height/2+panY-py*zoom*canvas.width*.72, depth};
  };
  const draw = () => {
    ctx.clearRect(0,0,canvas.width,canvas.height); ctx.fillStyle='#fbfdff'; ctx.fillRect(0,0,canvas.width,canvas.height);
    ctx.strokeStyle='#dbe5ef'; ctx.lineWidth=1; ctx.beginPath(); ctx.moveTo(18,canvas.height-24); ctx.lineTo(canvas.width-18,canvas.height-24); ctx.stroke();
    if (mode==='mesh') {
      for(let i=0;i<points.length;i+=3){ const q=points.slice(i,i+3).map(project); ctx.beginPath();ctx.moveTo(q[0].x,q[0].y);ctx.lineTo(q[1].x,q[1].y);ctx.lineTo(q[2].x,q[2].y);ctx.closePath();ctx.fillStyle='rgba(57,120,185,.10)';ctx.fill();ctx.strokeStyle='#3978b9';ctx.stroke(); }
    } else {
      const sorted=points.map((p,i)=>({p,i,q:project([p.x,p.y,p.z])})).sort((a,b)=>a.q.depth-b.q.depth);
      sorted.forEach(({p,i,q})=>{const t=(p.T-tempMin)/(tempMax-tempMin||1);ctx.beginPath();ctx.arc(q.x,q.y,5,0,Math.PI*2);ctx.fillStyle=`hsl(${220-220*t},80%,48%)`;ctx.fill();if(i===selected){ctx.strokeStyle='#111';ctx.lineWidth=2;ctx.stroke();}});
    }
    ctx.fillStyle='#52677d';ctx.font='12px Arial';ctx.fillText('Drag: rotate · Shift+drag: pan · Wheel: zoom · Double-click: fit',12,18);
  };
  canvas.onpointerdown=e=>{dragging=true;lastX=e.offsetX;lastY=e.offsetY;canvas.setPointerCapture(e.pointerId);};
  canvas.onpointerup=()=>dragging=false;
  canvas.onpointermove=e=>{if(!dragging)return;const dx=e.offsetX-lastX,dy=e.offsetY-lastY;lastX=e.offsetX;lastY=e.offsetY;if(e.shiftKey){panX+=dx;panY+=dy;}else{yaw+=dx*.009;pitch=Math.max(-1.45,Math.min(1.45,pitch+dy*.009));}draw();};
  canvas.onwheel=e=>{e.preventDefault();zoom=Math.max(.25,Math.min(8,zoom*(e.deltaY<0?1.1:.9)));draw();};
  canvas.ondblclick=()=>{yaw=.65;pitch=-.35;zoom=1;panX=panY=0;draw();};
  canvas.onclick=e=>{if(mode!=='cloud')return;let best=Infinity,idx=-1;points.forEach((p,i)=>{const q=project([p.x,p.y,p.z]),d=(q.x-e.offsetX)**2+(q.y-e.offsetY)**2;if(d<best){best=d;idx=i;}});if(best<900){selected=idx;$('csv-status').textContent=`Selected node ${points[idx].id}: (${points[idx].x}, ${points[idx].y}, ${points[idx].z}) m · ${points[idx].T} °C`;draw();}};
  draw(); return draw;
}
let tempMin=0,tempMax=1;

async function uploadGeometry(kind, file) {
  const target = kind==='stl'?'stl-status':'csv-status';
  try {
    if(file.size>20*1024*1024) throw new Error('File exceeds the documented 20 MiB limit.');
    const result=await api(`/api/geometry/${kind}`,{method:'POST',headers:{'Content-Type':'text/plain'},body:await file.text()});
    if(kind==='stl'){
      $('stl-info').style.display='grid';$('stl-triangles').textContent=result.triangles;
      ['x','y','z'].forEach((a,i)=>$(`stl-${a}range`).textContent=`${fmt(result.bounds[a][0],3)} … ${fmt(result.bounds[a][1],3)}`);
      create3DCanvas($('mesh-canvas'),result.vertices,'mesh');$('mesh-hint').style.display='block';
      setStatus(target,`C++ validation passed: ${result.triangles} triangles · ${file.name}`);
    } else {
      $('csv-info').style.display='grid';$('csv-nodes').textContent=result.nodes_count;$('csv-tmin').textContent=`${fmt(result.temp_min,2)} °C`;$('csv-tmax').textContent=`${fmt(result.temp_max,2)} °C`;
      tempMin=result.temp_min;tempMax=result.temp_max;create3DCanvas($('cloud-canvas'),result.nodes,'cloud');$('cloud-legend').style.display='flex';
      setStatus(target,`C++ validation passed: ${result.nodes_count} nodes · ${file.name}`);
    }
  } catch(e) { setStatus(target,e.message,false); }
}
$('stl-input').addEventListener('change',e=>e.target.files[0]&&uploadGeometry('stl',e.target.files[0]));
$('csv-input').addEventListener('change',e=>e.target.files[0]&&uploadGeometry('csv',e.target.files[0]));
for(const [id,kind] of [['stl-drop','stl'],['csv-drop','csv']]){
 const el=$(id);el.addEventListener('dragover',e=>{e.preventDefault();el.classList.add('dragover');});el.addEventListener('dragleave',()=>el.classList.remove('dragover'));
 el.addEventListener('drop',e=>{e.preventDefault();el.classList.remove('dragover');const f=e.dataTransfer.files[0];if(f)uploadGeometry(kind,f);});
}

function drawLineChart(canvas, series, title, yLabel) {
  const w=canvas.width=Math.max(500,canvas.clientWidth||500),h=canvas.height=300,ctx=canvas.getContext('2d');ctx.clearRect(0,0,w,h);ctx.fillStyle='#fff';ctx.fillRect(0,0,w,h);
  const all=series.flatMap(s=>s.values.map(p=>p.y)),xmin=Math.min(...series.flatMap(s=>s.values.map(p=>p.x))),xmax=Math.max(...series.flatMap(s=>s.values.map(p=>p.x)),xmin+1),ymin=Math.min(...all),ymax=Math.max(...all);const pad={l:58,r:20,t:34,b:42};
  const X=x=>pad.l+(x-xmin)/(xmax-xmin)*(w-pad.l-pad.r),Y=y=>h-pad.b-(y-ymin)/(ymax-ymin||1)*(h-pad.t-pad.b);
  ctx.strokeStyle='#e5ebf2';ctx.fillStyle='#5b6e82';ctx.font='11px Arial';for(let i=0;i<=4;i++){const y=pad.t+i*(h-pad.t-pad.b)/4;ctx.beginPath();ctx.moveTo(pad.l,y);ctx.lineTo(w-pad.r,y);ctx.stroke();ctx.fillText((ymax-(ymax-ymin)*i/4).toPrecision(4),5,y+4);}
  ctx.fillText(title,pad.l,18);ctx.fillText(yLabel,5,30);
  const palette=['#2463a6','#e17b32','#24966b','#8a58a8','#b34d65'];series.forEach((s,j)=>{ctx.beginPath();s.values.forEach((p,i)=>{if(i===0)ctx.moveTo(X(p.x),Y(p.y));else ctx.lineTo(X(p.x),Y(p.y));});ctx.strokeStyle=palette[j%palette.length];ctx.lineWidth=2;ctx.stroke();ctx.fillStyle=palette[j%palette.length];ctx.fillText(s.name,pad.l+j*130,h-12);});
}
function drawBarChart(canvas, rows, key, title) {
 const w=canvas.width=Math.max(400,canvas.clientWidth||400),h=280,ctx=canvas.getContext('2d');ctx.clearRect(0,0,w,h);ctx.fillStyle='#fff';ctx.fillRect(0,0,w,h);ctx.fillStyle='#243b53';ctx.font='13px Arial';ctx.fillText(title,18,22);
 const max=Math.max(...rows.map(r=>r[key]),1e-9),bw=(w-60)/rows.length*.55;rows.forEach((r,i)=>{const x=42+i*(w-65)/rows.length,y=h-42-r[key]/max*(h-90);ctx.fillStyle='#3978b9';ctx.fillRect(x,y,bw,h-42-y);ctx.fillStyle='#334';ctx.fillText(String(r.workers),x+4,h-20);ctx.fillText(fmt(r[key],2),x,y-7);});ctx.fillStyle='#667';ctx.fillText('Workers',w-65,h-20);
}
async function runThermal(){const btn=$('run-thermal-btn');btn.disabled=true;$('thermal-loading').style.display='block';try{const data=await api('/api/thermal');$('thermal-results').style.display='block';window.thermalData=data;const meshes=data.meshes;drawLineChart($('thermal-chart'),[{name:'FE linear solution N=80',values:meshes[3].nodes.map(p=>({x:p.x,y:p.T_fem}))},{name:'Analytical',values:Array.from({length:101},(_,i)=>({x:i/100,y:100-80*i/100+500*(i/100)*(1-i/100)}))}],'Rod temperature distribution','Temperature (°C)');drawLineChart($('convergence-chart'),[{name:'Maximum midpoint error',values:meshes.map(m=>({x:m.n,y:m.max_midpoint_error_C}))}],'Mesh convergence','Error (°C)');showThermalTab(10,document.querySelector('#thermal-tabs .tab-btn'));}catch(e){alert(`Thermal analysis failed: ${e.message}`);}finally{btn.disabled=false;$('thermal-loading').style.display='none';}}
function showThermalTab(n,button){if(!window.thermalData)return;document.querySelectorAll('#thermal-tabs .tab-btn').forEach(b=>b.classList.remove('active'));if(button)button.classList.add('active');const mesh=window.thermalData.meshes.find(m=>m.n===n);$('thermal-tbody').innerHTML=mesh.nodes.map(p=>`<tr><td>${fmt(p.x,4)}</td><td>${fmt(p.T_fem,6)}</td><td>${fmt(p.T_exact,6)}</td><td>${fmt(p.error,8)}</td></tr>`).join('');$('thermal-csv-link').href=`/api/results/thermal_N${n}.csv`;}
async function runBenchmark(){const btn=$('run-bench-btn');btn.disabled=true;$('bench-loading').style.display='block';try{const cases=Number($('bench-cases').value),elements=Number($('bench-elements').value),seed=Number($('bench-seed').value);if(!Number.isInteger(cases)||cases<1||cases>100000||!Number.isInteger(elements)||elements<2||elements>10000||!Number.isInteger(seed)||seed<0)throw new Error('Cases must be 1–100000, elements 2–10000, and seed a non-negative integer.');const data=await api(`/api/benchmark?cases=${cases}&elements=${elements}&seed=${seed}`);$('bench-results').style.display='block';const rows=data.rows;drawBarChart($('bench-time-chart'),rows,'median_ms','Median compute time (ms)');drawBarChart($('bench-speedup-chart'),rows,'speedup','Speedup vs serial');$('bench-tbody').innerHTML=rows.map(r=>`<tr><td>${r.workers}</td><td>${fmt(r.median_ms,3)}</td><td>${fmt(r.speedup,3)}×</td><td>${fmt(r.efficiency*100,1)}%</td><td>${r.max_abs_difference_C.toExponential(2)}</td><td>${r.logical_cpus}</td></tr>`).join('');$('bench-note').textContent=`${rows[0].cases} deterministic cases · ${rows[0].elements} elements · seed ${rows[0].seed} · 5 measured repetitions; maximum serial/parallel difference ${Math.max(...rows.map(r=>r.max_abs_difference_C)).toExponential(2)} °C.`;}catch(e){alert(`Benchmark failed: ${e.message}`);}finally{btn.disabled=false;$('bench-loading').style.display='none';}}
async function discoverDevices(action='',id=-1){$('hw-loading').style.display='block';try{const q=action?`?action=${encodeURIComponent(action)}&id=${id}`:'';const data=await api('/api/hardware'+q);$('hw-devices').style.display='grid';$('hw-devices').innerHTML=data.devices.map(d=>`<article class="card"><b>${d.type}</b><h3>${d.name}</h3><p>${d.state}</p><small>${d.capabilities}<br>${d.message}</small><div class="device-actions"><button class="btn btn-outline" onclick="discoverDevices('connect',${d.id})">Connect</button><button class="btn btn-outline" onclick="discoverDevices('telemetry',${d.id})">Telemetry</button>${d.type==='JTAG adapter'?`<button class="btn btn-outline" onclick="discoverDevices('retry',${d.id})">Retry timeout</button>`:''}<button class="btn btn-outline" onclick="discoverDevices('disconnect',${d.id})">Disconnect</button></div></article>`).join('');}catch(e){alert(`Device simulation failed: ${e.message}`);}finally{$('hw-loading').style.display='none';}}
async function runBackprop(){const btn=$('run-bp-btn');btn.disabled=true;$('bp-loading').style.display='block';try{const d=await api('/api/backprop'),r=d.first_step;$('bp-results').style.display='block';[['bp-h',r.h],['bp-pred',r.prediction],['bp-loss',r.loss],['bp-dw',r.gradients.dJ_dw],['bp-db',r.gradients.dJ_db],['bp-dv',r.gradients.dJ_dv],['bp-dc',r.gradients.dJ_dc],['bp-fderr',r.max_fd_error]].forEach(([id,v])=>$(id).textContent=Number(v).toExponential(5));const badge=$('bp-fd-check');badge.style.display='inline-block';badge.textContent=r.max_fd_error<=1e-5?'PASS: all four gradients verified by central finite differences':'FAIL: gradient check exceeds tolerance';badge.className=r.max_fd_error<=1e-5?'badge badge-pass':'badge badge-fail';drawLineChart($('bp-chart'),[{name:'Training loss',values:d.history.map(p=>({x:p.step,y:p.loss}))}],'Loss over 100 gradient-descent updates','Loss');}catch(e){alert(`Backpropagation failed: ${e.message}`);}finally{btn.disabled=false;$('bp-loading').style.display='none';}}
window.scrollToSection=scrollToSection;window.runThermal=runThermal;window.showThermalTab=showThermalTab;window.runBenchmark=runBenchmark;window.discoverDevices=discoverDevices;window.runBackprop=runBackprop;
