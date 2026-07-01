typedef double DATA_TYPE;

void kernel_fdtd_2d(int tmax, int nx, int ny,
                    DATA_TYPE *ex, DATA_TYPE *ey, DATA_TYPE *hz, DATA_TYPE *_fict_)
    __attribute__((export_name("kernel_fdtd_2d")));

void kernel_fdtd_2d(int tmax, int nx, int ny,
                    DATA_TYPE *ex, DATA_TYPE *ey, DATA_TYPE *hz, DATA_TYPE *_fict_) {
    int t, i, j;
    for (t = 0; t < tmax; t++) {
        for (j = 0; j < ny; j++)
            ey[0*ny+j] = _fict_[t];
        for (i = 1; i < nx; i++)
            for (j = 0; j < ny; j++)
                ey[i*ny+j] = ey[i*ny+j] - 0.5*(hz[i*ny+j]-hz[(i-1)*ny+j]);
        for (i = 0; i < nx; i++)
            for (j = 1; j < ny; j++)
                ex[i*ny+j] = ex[i*ny+j] - 0.5*(hz[i*ny+j]-hz[i*ny+j-1]);
        for (i = 0; i < nx-1; i++)
            for (j = 0; j < ny-1; j++)
                hz[i*ny+j] = hz[i*ny+j] - 0.7*(ex[i*ny+j+1] - ex[i*ny+j] +
                                                 ey[(i+1)*ny+j] - ey[i*ny+j]);
    }
}
