# Backpropagation numerical reference — candidate must handwrite separately

Assignment values: x=1, target y=2, w=0.5, b=0, v=1, c=0, learning rate η=0.1.

## Forward pass

- z = wx+b = 0.5
- h = tanh(z) = 0.46211715726000974
- ŷ = vh+c = 0.46211715726000974
- e = ŷ−y = −1.5378828427399902
- J = 0.5e² = 1.1825418189970167

## Chain rule

- ∂J/∂c = e
- ∂J/∂v = e·h
- ∂J/∂h = e·v
- ∂h/∂z = 1−h²
- δ = e·v·(1−h²)
- ∂J/∂w = δ·x
- ∂J/∂b = δ

Initial gradients in `[w,b,v,c]` order:

- ∂J/∂w = −1.2094644752400612
- ∂J/∂b = −1.2094644752400612
- ∂J/∂v = −0.7106820474859469
- ∂J/∂c = −1.5378828427399902

## One simultaneous update

Use all four gradients from the original parameters: `θ_new = θ_old − η∂J/∂θ`.

- w_new = 0.6209464475240061
- b_new = 0.12094644752400613
- v_new = 1.0710682047485947
- c_new = 0.15378828427399904

Negative gradients make the corresponding parameters increase under gradient descent. The C++ implementation checks each initial gradient using central finite differences `(J(θ+ε)−J(θ−ε))/(2ε)`, ε=10⁻⁶. The observed maximum absolute error is recorded in `validation_log.md`.

**Submission requirement:** reproduce the forward pass, chain-rule derivation and simultaneous update by hand on a legible appendix of no more than two pages, then scan/photograph it. This typed reference is not a substitute for the candidate-authored handwritten evidence.
