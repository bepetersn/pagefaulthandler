
import sys
import numpy as np

if __name__ == "__main__":
    results = np.array([float(v) for v in sys.argv[1:]])
    print(results)
    print(f'mean: {np.mean(results)}')
    print(f'median: {np.median(results)}')
    print(f'min: {np.amin(results)}')
    print(f'max: {np.amax(results)}')
    print(f'variance: {np.var(results)}')
