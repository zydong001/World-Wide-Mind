<<<<<<< HEAD
clear all; close all; clc;
load('32ChannelEEG.mat');
nSamples = 500;
nChannels = 1;
data = A(1:nSamples, 1:nChannels);
tmp = data;
m = 4;
cA = cell(m,1);
cD = cell(m,1);
filter = 'db4';
for i = 1:m
    [cA{i},cD{i}] = dwt(tmp,filter);
    tmp = cA{i};
end
cD1 = zeros(length(cA{1}),1);
cD2 = zeros(length(cA{2}),1);
cD3 = zeros(length(cA{3}),1);
cD4 = zeros(length(cA{4}),1);

dwt_1 = idwt(cA{1},cD1,filter);

error1 = sqrt(sum((data - dwt_1).^2)./sum(data.^2)) * 100;

dwt_20 = idwt(cA{2},cD2,filter);
dwt_21 = idwt(dwt_20(1:end-1),cD1,filter);
error2 = sqrt(sum((data - dwt_21).^2)./sum(data.^2)) * 100;

dwt_30 = idwt(cA{3},cD3,filter);
dwt_31 = idwt(dwt_30,cD2,filter);
dwt_32 = idwt(dwt_31(1:end-1),cD1,filter);
error3 = sqrt(sum((data - dwt_32).^2)./sum(data.^2)) * 100;

dwt_40 = idwt(cA{4},cD4,filter);
dwt_41 = idwt(dwt_40,cD3,filter);
dwt_42 = idwt(dwt_41,cD2,filter);
dwt_43 = idwt(dwt_42(1:end-1),cD1,filter);
error4 = sqrt(sum((data - dwt_43).^2)./sum(data.^2)) * 100;

subplot(3,2,1);
plot(data);
title('CR = 1 (Raw data)');
grid on;grid minor;ylim([-200, 200]);
subplot(3,2,2);
plot(dwt_1);
title('CR = 2');
grid on;grid minor;ylim([-200, 200]);
subplot(3,2,3);
plot(dwt_21);
title('CR = 4');
grid on;grid minor;ylim([-200, 200]);
subplot(3,2,4);
plot(dwt_32);
title('CR = 8');
grid on;grid minor;ylim([-200, 200]);
subplot(3,2,5);
plot(dwt_43);
title('CR = 16');
=======
clear all; close all; clc;
load('32ChannelEEG.mat');
nSamples = 500;
nChannels = 1;
data = A(1:nSamples, 1:nChannels);
tmp = data;
m = 4;
cA = cell(m,1);
cD = cell(m,1);
filter = 'db4';
for i = 1:m
    [cA{i},cD{i}] = dwt(tmp,filter);
    tmp = cA{i};
end
cD1 = zeros(length(cA{1}),1);
cD2 = zeros(length(cA{2}),1);
cD3 = zeros(length(cA{3}),1);
cD4 = zeros(length(cA{4}),1);

dwt_1 = idwt(cA{1},cD1,filter);

error1 = sqrt(sum((data - dwt_1).^2)./sum(data.^2)) * 100;

dwt_20 = idwt(cA{2},cD2,filter);
dwt_21 = idwt(dwt_20(1:end-1),cD1,filter);
error2 = sqrt(sum((data - dwt_21).^2)./sum(data.^2)) * 100;

dwt_30 = idwt(cA{3},cD3,filter);
dwt_31 = idwt(dwt_30,cD2,filter);
dwt_32 = idwt(dwt_31(1:end-1),cD1,filter);
error3 = sqrt(sum((data - dwt_32).^2)./sum(data.^2)) * 100;

dwt_40 = idwt(cA{4},cD4,filter);
dwt_41 = idwt(dwt_40,cD3,filter);
dwt_42 = idwt(dwt_41,cD2,filter);
dwt_43 = idwt(dwt_42(1:end-1),cD1,filter);
error4 = sqrt(sum((data - dwt_43).^2)./sum(data.^2)) * 100;

subplot(3,2,1);
plot(data);
title('CR = 1 (Raw data)');
grid on;grid minor;ylim([-200, 200]);
subplot(3,2,2);
plot(dwt_1);
title('CR = 2');
grid on;grid minor;ylim([-200, 200]);
subplot(3,2,3);
plot(dwt_21);
title('CR = 4');
grid on;grid minor;ylim([-200, 200]);
subplot(3,2,4);
plot(dwt_32);
title('CR = 8');
grid on;grid minor;ylim([-200, 200]);
subplot(3,2,5);
plot(dwt_43);
title('CR = 16');
>>>>>>> a8e4fc521e99e882afcff243c6c8f5bff6ecfcb0
grid on;grid minor;ylim([-200, 200]);