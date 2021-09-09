clear all; close all; clc;
load('32ChannelEEG.mat');
nSamples = 500;
nChannels = 1;
data = A(1:nSamples, 1:nChannels);
coeff = dct(data);
dct_1 = idct(coeff);
error1 = sqrt(sum((data - dct_1).^2)./sum(data.^2)) * 100;

coeff_2 = coeff;
coeff_2(nSamples/2 + 1:end) = 0;
dct_2 = idct(coeff_2);
error2 = sqrt(sum((data - dct_2).^2)./sum(data.^2)) * 100;

coeff_4 = coeff;
coeff_4(nSamples/4 + 1:end) = 0;
dct_4 = idct(coeff_4);
error4 = sqrt(sum((data - dct_4).^2)./sum(data.^2)) * 100;

coeff_6 = coeff;
coeff_6(nSamples/6 + 1:end) = 0;
dct_6 = idct(coeff_6);
error6 = sqrt(sum((data - dct_6).^2)./sum(data.^2)) * 100;

coeff_8 = coeff;
coeff_8(nSamples/8 + 1:end) = 0;
dct_8 = idct(coeff_8);
error8 = sqrt(sum((data - dct_8).^2)./sum(data.^2)) * 100;

coeff_10 = coeff;
coeff_10(nSamples/10 + 1:end) = 0;
dct_10= idct(coeff_10);
error10 = sqrt(sum((data - dct_10).^2)./sum(data.^2)) * 100;

coeff_12 = coeff;
coeff_12(nSamples/12 + 1:end) = 0;
dct_12 = idct(coeff_12);
error12 = sqrt(sum((data - dct_12).^2)./sum(data.^2)) * 100;

coeff_14 = coeff;
coeff_14(nSamples/14 + 1:end) = 0;
dct_14 = idct(coeff_14);
error14 = sqrt(sum((data - dct_14).^2)./sum(data.^2)) * 100;

coeff_16 = coeff;
coeff_16(nSamples/16 + 1:end) = 0;
dct_16 = idct(coeff_16);
error16 = sqrt(sum((data - dct_16).^2)./sum(data.^2)) * 100;

% dct_2 = idct(coeff(1:nSamples/2));
% dct_4 = idct(coeff(1:nSamples/4));
% dct_6 = idct(coeff(1:nSamples/6));
% dct_8 = idct(coeff(1:nSamples/8));
% dct_10 = idct(coeff(1:nSamples/10));
% dct_12 = idct(coeff(1:nSamples/12));
% dct_14 = idct(coeff(1:nSamples/14));
% dct_16 = idct(coeff(1:nSamples/16));
subplot(3,3,1);
plot(dct_1);
title('CR = 1 (Raw data)');
grid on;grid minor;ylim([-200, 200]);
subplot(3,3,2);
plot(dct_2);
title('CR = 2');
grid on;grid minor;ylim([-200, 200]);
subplot(3,3,3);
plot(dct_4);
title('CR = 4');
grid on;grid minor;ylim([-200, 200]);
subplot(3,3,4);
plot(dct_6);
title('CR = 6');
grid on;grid minor;ylim([-200, 200]);
subplot(3,3,5);
plot(dct_8);
title('CR = 8');
grid on;grid minor;ylim([-200, 200]);
subplot(3,3,6);
plot(dct_10);
title('CR = 10');
grid on;grid minor;ylim([-200, 200]);
subplot(3,3,7);
plot(dct_12);
title('CR = 12');
grid on;grid minor;ylim([-200, 200]);
subplot(3,3,8);
plot(dct_14);
title('CR = 14');
grid on;grid minor;ylim([-200, 200]);
subplot(3,3,9);
plot(dct_16);
title('CR = 16');
grid on;grid minor;ylim([-200, 200]);