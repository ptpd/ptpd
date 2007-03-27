#!/usr/bin/octave -qf

# the IIR filter
s = 2^4;
a = [ s -(s-1) ];
b = [ 1/2 1/2 ];

[h w] = freqz(b, a, 100000);

subplot(211);
plot(w/pi,abs(h),";;");
axis();
#plot(w/pi,20*log(abs(h)));
#axis([0 1 -120 0]);
#semilogx(w/pi,20*log(abs(h)),";;");
#axis([1e-3 1 -120 0]);
ylabel("gain");

subplot(212);
plot(w/pi,unwrap(angle(h)),";;");
axis([0 1 -pi 0]);
#semilogx(w/pi,unwrap(angle(h)),";;");
#axis([1e-3 1 -pi 0]);
ylabel("phase (rad)");
xlabel("frequency");
replot;

pause;

