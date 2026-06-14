# Python Benchmark — Fibonacci recursivo (comparación con KEM)
import time

def fibonacci(n):
    if n < 2:
        return n
    return fibonacci(n - 1) + fibonacci(n - 2)

start = time.perf_counter_ns()
resultado = fibonacci(35)
elapsed = time.perf_counter_ns() - start

print(f"resultado={resultado}")
print(f"tiempo={elapsed // 1000} µs")
