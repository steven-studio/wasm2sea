typedef double DATA_TYPE;

void kernel_adi(int tsteps, int n,
                DATA_TYPE *u, DATA_TYPE *v, DATA_TYPE *p, DATA_TYPE *q)
    __attribute__((export_name("kernel_adi")));

void kernel_adi(int tsteps, int n,
                DATA_TYPE *u, DATA_TYPE *v, DATA_TYPE *p, DATA_TYPE *q) {
    int t, i, j;
    DATA_TYPE DX, DY, DT;
    DATA_TYPE B1, B2;
    DATA_TYPE mul1, mul2;
    DATA_TYPE a, b, c, d, e, f;

    DX = 1.0 / (DATA_TYPE)n;
    DY = 1.0 / (DATA_TYPE)n;
    DT = 1.0 / (DATA_TYPE)tsteps;
    B1 = 2.0;
    B2 = 1.0;
    mul1 = B1 * DT / (DX * DX);
    mul2 = B2 * DT / (DY * DY);

    a = -mul1 / 2.0;
    b = 1.0 + mul1;
    c = a;
    d = -mul2 / 2.0;
    e = 1.0 + mul2;
    f = d;

    for (t = 1; t <= tsteps; t++) {
        for (i = 1; i < n-1; i++) {
            v[0*n+i] = 1.0;
            p[i*n+0] = 0.0;
            q[i*n+0] = v[0*n+i];
            for (j = 1; j < n-1; j++) {
                p[i*n+j] = -c / (a*p[i*n+j-1]+b);
                q[i*n+j] = (-d*u[j*n+i-1]+(1.0+2.0*d)*u[j*n+i] - f*u[j*n+i+1]-a*q[i*n+j-1])/(a*p[i*n+j-1]+b);
            }
            v[(n-1)*n+i] = 1.0;
            for (j = n-2; j >= 1; j--) {
                v[j*n+i] = p[i*n+j] * v[(j+1)*n+i] + q[i*n+j];
            }
        }
        for (i = 1; i < n-1; i++) {
            u[i*n+0] = 1.0;
            p[i*n+0] = 0.0;
            q[i*n+0] = u[i*n+0];
            for (j = 1; j < n-1; j++) {
                p[i*n+j] = -f / (d*p[i*n+j-1]+e);
                q[i*n+j] = (-a*v[(i-1)*n+j]+(1.0+2.0*a)*v[i*n+j] - c*v[(i+1)*n+j]-d*q[i*n+j-1])/(d*p[i*n+j-1]+e);
            }
            u[i*n+(n-1)] = 1.0;
            for (j = n-2; j >= 1; j--) {
                u[i*n+j] = p[i*n+j] * u[i*n+j+1] + q[i*n+j];
            }
        }
    }
}
