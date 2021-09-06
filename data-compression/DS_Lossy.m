clear all; close all; clc;
%A = readmatrix('subj1_series9_data.csv','range',[2,2,501,33]);
load('32ChannelEEG.mat');
nSamples = 500;
nChannels = 1;
data = A(1:nSamples, 1:nChannels);

data2 = downsample(data,2);
data2 = upsample(data2,2);
error2 = sqrt(sum((data - data2).^2)./sum(data.^2)) * 100;

data4 = downsample(data,4);
data4 = upsample(data4,4);
error4 = sqrt(sum((data - data4).^2)./sum(data.^2)) * 100;

data6 = downsample(data,6);
data6 = upsample(data6,6);
data6 = data6(1:500);
error6 = sqrt(sum((data - data6).^2)./sum(data.^2)) * 100;

data8 = downsample(data,8);
data8 = upsample(data8,8);
data8 = data8(1:500);
error8 = sqrt(sum((data - data8).^2)./sum(data.^2)) * 100;

data10 = downsample(data,10);
data10 = upsample(data10,10);
error10 = sqrt(sum((data - data10(1:500)).^2)./sum(data.^2)) * 100;

data12 = downsample(data,12);
data12 = upsample(data12,12);
data12 = data12(1:500);
error12 = sqrt(sum((data - data12).^2)./sum(data.^2)) * 100;

data14 = downsample(data,14);
data14 = upsample(data14,14);
data14 = data14(1:500);
error14 = sqrt(sum((data - data14).^2)./sum(data.^2)) * 100;

data16 = downsample(data,16);
data16 = upsample(data16,16);
data16 = data16(1:500);
error16 = sqrt(sum((data - data16).^2)./sum(data.^2)) * 100;

subplot(3,3,1);
plot(data);
title('CR = 1 (Raw data)');
grid on;grid minor;ylim([-200, 200]);
subplot(3,3,2);
plot(data2);
title('CR = 2');
grid on;grid minor;ylim([-200, 200]);
subplot(3,3,3);
plot(data4);
title('CR = 4');
grid on;grid minor;ylim([-200, 200]);
subplot(3,3,4);
plot(data6);
title('CR = 6');
grid on;grid minor;ylim([-200, 200]);
subplot(3,3,5);
plot(data8);
title('CR = 8');
grid on;grid minor;ylim([-200, 200]);
subplot(3,3,6);
plot(data10);
title('CR = 10');
grid on;grid minor;ylim([-200, 200]);
subplot(3,3,7);
plot(data12);
title('CR = 12');
grid on;grid minor;ylim([-200, 200]);
subplot(3,3,8);
plot(data14);
title('CR = 14');
grid on;grid minor;ylim([-200, 200]);
subplot(3,3,9);
plot(data16);
title('CR = 16');
grid on;grid minor; ylim([-200, 200]);


%data_100_channel_3_raw = data;
% data_100_channel_3_raw_ratio_2 = A(1:2:nSamples, 1:nChannels);
% data_100_channel_3_raw_ratio_4 = A(1:4:nSamples, 1:nChannels);
% data_100_channel_3_raw_ratio_6 = A(1:6:nSamples, 1:nChannels);
% data_100_channel_3_raw_ratio_8 = A(1:8:nSamples, 1:nChannels);
% data_100_channel_3_raw_ratio_10 = A(1:10:nSamples, 1:nChannels);
% data_100_channel_3_raw_ratio_12 = A(1:12:nSamples, 1:nChannels);
% data_100_channel_3_raw_ratio_14 = A(1:14:nSamples, 1:nChannels);
% data_100_channel_3_raw_ratio_16 = A(1:16:nSamples, 1:nChannels);
% subplot(3,3,1);
% plot(data_100_channel_3_raw);
% title('CR = 1');
% subplot(3,3,2);
% plot(data_100_channel_3_raw_ratio_2);
% title('CR = 2');
% subplot(3,3,3);
% plot(data_100_channel_3_raw_ratio_4);
% title('CR = 4');
% subplot(3,3,4);
% plot(data_100_channel_3_raw_ratio_6);
% title('CR = 6');
% subplot(3,3,5);
% plot(data_100_channel_3_raw_ratio_8);
% title('CR = 8');
% subplot(3,3,6);
% plot(data_100_channel_3_raw_ratio_10);
% title('CR = 10');
% subplot(3,3,7);
% plot(data_100_channel_3_raw_ratio_12);
% title('CR = 12');
% subplot(3,3,8);
% plot(data_100_channel_3_raw_ratio_14);
% title('CR = 14');
% subplot(3,3,9);
% plot(data_100_channel_3_raw_ratio_16);
% title('CR = 16');
