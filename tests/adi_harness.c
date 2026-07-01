#include <stdio.h>
#include <stdint.h>
#include <math.h>

static uint8_t wasm_memory[1048576];
void kernel_adi(uintptr_t __mem, int32_t tsteps, int32_t n,
                int32_t u_off, int32_t v_off, int32_t p_off, int32_t q_off);

static void ref_adi(int tsteps, int n, double *u, double *v, double *p, double *q) {
    int t, i, j;
    double DX, DY, DT, B1, B2, mul1, mul2, a, b, c, d, e, f;
    DX = 1.0/(double)n; DY = 1.0/(double)n; DT = 1.0/(double)tsteps;
    B1 = 2.0; B2 = 1.0;
    mul1 = B1*DT/(DX*DX); mul2 = B2*DT/(DY*DY);
    a = -mul1/2.0; b = 1.0+mul1; c = a;
    d = -mul2/2.0; e = 1.0+mul2; f = d;
    for (t=1; t<=tsteps; t++) {
        for (i=1; i<n-1; i++) {
            v[0*n+i]=1.0; p[i*n+0]=0.0; q[i*n+0]=v[0*n+i];
            for (j=1; j<n-1; j++) {
                p[i*n+j] = -c/(a*p[i*n+j-1]+b);
                q[i*n+j] = (-d*u[j*n+i-1]+(1.0+2.0*d)*u[j*n+i]-f*u[j*n+i+1]-a*q[i*n+j-1])/(a*p[i*n+j-1]+b);
            }
            v[(n-1)*n+i]=1.0;
            for (j=n-2; j>=1; j--) v[j*n+i] = p[i*n+j]*v[(j+1)*n+i]+q[i*n+j];
        }
        for (i=1; i<n-1; i++) {
            u[i*n+0]=1.0; p[i*n+0]=0.0; q[i*n+0]=u[i*n+0];
            for (j=1; j<n-1; j++) {
                p[i*n+j] = -f/(d*p[i*n+j-1]+e);
                q[i*n+j] = (-a*v[(i-1)*n+j]+(1.0+2.0*a)*v[i*n+j]-c*v[(i+1)*n+j]-d*q[i*n+j-1])/(d*p[i*n+j-1]+e);
            }
            u[i*n+(n-1)]=1.0;
            for (j=n-2; j>=1; j--) u[i*n+j] = p[i*n+j]*u[i*n+j+1]+q[i*n+j];
        }
    }
}

int main() {
    int tsteps=2, n=5;
    int base = 131072;
    int u_off=base, v_off=u_off+n*n*8, p_off=v_off+n*n*8, q_off=p_off+n*n*8;

    double *u=(double*)(wasm_memory+u_off), *v=(double*)(wasm_memory+v_off);
    double *p=(double*)(wasm_memory+p_off), *q=(double*)(wasm_memory+q_off);
    double refu[25], refv[25], refp[25], refq[25];

    for (int i=0;i<n*n;i++){u[i]=(i%5)+1.0; refu[i]=(i%5)+1.0;}
    for (int i=0;i<n*n;i++){v[i]=0.0; refv[i]=0.0;}
    for (int i=0;i<n*n;i++){p[i]=0.0; refp[i]=0.0;}
    for (int i=0;i<n*n;i++){q[i]=0.0; refq[i]=0.0;}

    kernel_adi((uintptr_t)wasm_memory, tsteps, n, u_off, v_off, p_off, q_off);
    ref_adi(tsteps, n, refu, refv, refp, refq);

    int pass=1;
    for (int i=0;i<n*n;i++) {
        if (fabs(u[i]-refu[i])>1e-6){pass=0;printf("MISMATCH u[%d]: %.6f ref=%.6f\n",i,u[i],refu[i]);}
    }
    if (pass) printf("PASS adi differential test\n");
    else printf("FAIL\n");
    return 0;
}
