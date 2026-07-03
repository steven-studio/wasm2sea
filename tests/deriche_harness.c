#include <stdio.h>
#include <stdint.h>
#include <math.h>

static uint8_t wasm_memory[1048576];
void test(uintptr_t __mem, int32_t w, int32_t h, double alpha,
         int32_t imgIn_off, int32_t imgOut_off, int32_t y1_off, int32_t y2_off);

static void ref_deriche(int w, int h, double alpha,
                        double *imgIn, double *imgOut, double *y1, double *y2) {
    int i, j;
    double xm1, tm1, ym1, ym2, xp1, xp2, tp1, tp2, yp1, yp2;
    double k, a1, a2, a3, a4, a5, a6, a7, a8, b1, b2, c1, c2;

    k = (1.0-exp(-alpha))*(1.0-exp(-alpha))/(1.0+2.0*alpha*exp(-alpha)-exp(2.0*alpha));
    a1 = a5 = k;
    a2 = a6 = k*exp(-alpha)*(alpha-1.0);
    a3 = a7 = k*exp(-alpha)*(alpha+1.0);
    a4 = a8 = -k*exp(-2.0*alpha);
    b1 = pow(2.0, -alpha);
    b2 = -exp(-2.0*alpha);
    c1 = c2 = 1;

    for (i=0; i<w; i++) {
        ym1 = 0.0; ym2 = 0.0; xm1 = 0.0;
        for (j=0; j<h; j++) {
            y1[i*h+j] = a1*imgIn[i*h+j] + a2*xm1 + b1*ym1 + b2*ym2;
            xm1 = imgIn[i*h+j]; ym2 = ym1; ym1 = y1[i*h+j];
        }
    }
    for (i=0; i<w; i++) {
        yp1 = 0.0; yp2 = 0.0; xp1 = 0.0; xp2 = 0.0;
        for (j=h-1; j>=0; j--) {
            y2[i*h+j] = a3*xp1 + a4*xp2 + b1*yp1 + b2*yp2;
            xp2 = xp1; xp1 = imgIn[i*h+j]; yp2 = yp1; yp1 = y2[i*h+j];
        }
    }
    for (i=0; i<w; i++)
        for (j=0; j<h; j++)
            imgOut[i*h+j] = c1 * (y1[i*h+j] + y2[i*h+j]);

    for (j=0; j<h; j++) {
        tm1 = 0.0; ym1 = 0.0; ym2 = 0.0;
        for (i=0; i<w; i++) {
            y1[i*h+j] = a5*imgOut[i*h+j] + a6*tm1 + b1*ym1 + b2*ym2;
            tm1 = imgOut[i*h+j]; ym2 = ym1; ym1 = y1[i*h+j];
        }
    }
    for (j=0; j<h; j++) {
        tp1 = 0.0; tp2 = 0.0; yp1 = 0.0; yp2 = 0.0;
        for (i=w-1; i>=0; i--) {
            y2[i*h+j] = a7*tp1 + a8*tp2 + b1*yp1 + b2*yp2;
            tp2 = tp1; tp1 = imgOut[i*h+j]; yp2 = yp1; yp1 = y2[i*h+j];
        }
    }
    for (i=0; i<w; i++)
        for (j=0; j<h; j++)
            imgOut[i*h+j] = c2*(y1[i*h+j] + y2[i*h+j]);
}

int main() {
    int w=4, h=4;
    double alpha=0.5;
    int base = 131072;
    int imgIn_off=base, imgOut_off=imgIn_off+w*h*8, y1_off=imgOut_off+w*h*8, y2_off=y1_off+w*h*8;

    double *imgIn=(double*)(wasm_memory+imgIn_off), *imgOut=(double*)(wasm_memory+imgOut_off);
    double *y1=(double*)(wasm_memory+y1_off), *y2=(double*)(wasm_memory+y2_off);
    double refIn[16], refOut[16], refy1[16], refy2[16];

    for (int i=0;i<w*h;i++){double v=(i%5)+1.0; imgIn[i]=v; refIn[i]=v;}
    for (int i=0;i<w*h;i++){imgOut[i]=0.0; refOut[i]=0.0;}
    for (int i=0;i<w*h;i++){y1[i]=0.0; refy1[i]=0.0;}
    for (int i=0;i<w*h;i++){y2[i]=0.0; refy2[i]=0.0;}

    test((uintptr_t)wasm_memory, w, h, alpha, imgIn_off, imgOut_off, y1_off, y2_off);
    ref_deriche(w, h, alpha, refIn, refOut, refy1, refy2);

    int pass=1;
    for (int i=0;i<w*h;i++) {
        if (fabs(imgOut[i]-refOut[i])>1e-6){pass=0;printf("MISMATCH imgOut[%d]: %.6f ref=%.6f\n",i,imgOut[i],refOut[i]);}
    }
    if (pass) printf("PASS deriche differential test\n");
    else printf("FAIL\n");
    return 0;
}
