#!/usr/bin/octave -qf

printf("start\n");

load t;
load tr;

# time window to analyze
t_beg = 1;
t_fin = length(t);

# parameters for synthetic reference time
#start_time = round(t(t_beg));
#sample_interval = 1;

# parameters for histogram
h_beg =  -15e-6;
h_fin = 15e-6;
h_sz = 1e-6;

# parameters for allan variance
# tune for your computational power
tau_beg = 1;
tau_fin = length(t)/10;
tau_maxsamps = 100;

# ----------

printf("data loaded\n");

# create synthetic reference time
#tr = t;
#tr(t_beg) = start_time;
#for k = (t_beg+1):t_fin
#  
#  tr(k) = tr(k-1) + sample_interval;
#  
#endfor

# time offset computation
o = t - tr;

o_min = min(o(t_beg:t_fin));
o_max = max(o(t_beg:t_fin));
o_mean = mean(o(t_beg:t_fin));

# relative tick rate computation
r = t;
for k = (t_beg+1):t_fin
  
  r(k) = o(k) - o(k-1);
  
endfor

r_min = min(r((t_beg+1):t_fin));
r_max = max(r((t_beg+1):t_fin));
r_mean = mean(r((t_beg+1):t_fin));

figure;
h_bins = h_beg:h_sz:h_fin;
hist( o(t_beg:t_fin), h_bins, 1);

printf("histogram plotted\n");

figure;
subplot(211);
plot( t_beg:t_fin, o(t_beg:t_fin), "r;time offset;",
        [t_beg t_fin], [o_min o_min], "g;;",
        [t_beg t_fin], [o_max o_max], "g;;",
        [t_beg t_fin], [o_mean o_mean], "b;;");
subplot(212);
plot( (t_beg+1):t_fin, r((t_beg+1):t_fin), "r;relative tick rate;",
        [(t_beg+1) t_fin], [r_min r_min], "g;;",
        [(t_beg+1) t_fin], [r_max r_max], "g;;",
        [(t_beg+1) t_fin], [r_mean r_mean], "b;;")
replot;

printf("offset plotted\n");

# the allan vaiance computation from the IEEE 1588 spec
a = t;
for tau = tau_beg:tau_fin
  
  beg = t_beg;
  if (t_fin-2*tau) > tau_maxsamps
    fin = tau_maxsamps;
  else
    fin = (t_fin-2*tau);
  end
  
  a(tau) = sum((t(beg:fin) - 2*t((beg+tau):(fin+tau)) + t((beg+2*tau):(fin+2*tau))).^2);
  a(tau) /= 2*(fin-beg)*(tau^2);
  
endfor

figure;
loglog( tau_beg:tau_fin, a(tau_beg:tau_fin), ";allan variance;");
replot;

printf("variance plotted\n");
input("done\n");

