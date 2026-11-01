# Thermal FEA formulation
Governing equation: `-d/dx(k A dT/dx) = q A`. For a linear two-node element of length `h`, the element stiffness is `(kA/h)[[1,-1],[-1,1]]` and the consistent element load is `(qAh/2)[1,1]^T`. Assemble the global tridiagonal system and impose `T(0)=100 °C`, `T(L)=20 °C` by eliminating prescribed degrees of freedom and moving their contributions to the right-hand side.

Assumptions: steady state, one-dimensional conduction, constant isotropic conductivity, uniform cross-section, uniform volumetric generation, no lateral heat loss, perfect continuity between elements. Units: L m, A m², k W/(m K), q W/m³, T °C (temperature differences in K).

Analytical solution: `T_exact(x)=100-80x/L+[q/(2k)]x(L-x)` for L=1 m. FE midpoint temperature is the average of the two linear-element nodal temperatures. Since the exact solution is quadratic, midpoint interpolation error should decrease as the mesh is refined. Boundary temperatures are imposed exactly.

Debugging insight: applying a prescribed temperature only by overwriting the boundary row can leave incorrect boundary-column terms in the interior equations. Elimination must subtract `K_ib*T_b` from the interior right-hand side before solving.
