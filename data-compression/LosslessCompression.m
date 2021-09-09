<<<<<<< HEAD
clear all; close all; clc;
load('32ChannelEEG.mat');
% for i = 1:500
%     fprintf('%d, ',A(i,3));
% end
nSamples = 500;
nChannels = 32;

O = ones(nSamples, nChannels);
%S = zeros(nSamples, nChannels);
SD = zeros(nSamples, nChannels);

meanOfEachChannel = mean(A);
stdOfEachChannel = std(A);
S = (A - gmultiply(O,meanOfEachChannel))./gmultiply(O,stdOfEachChannel);

D(1 , :) = A(1 , :);
for i = 2 : nSamples
    D(i, :) = A(i, :) - A(i - 1, :);
end

SD(1 , :) = S(1 , :);
for i = 2 : nSamples
    SD(i, :) = S(i, :) - S(i - 1, :);
end

[CR1, flag1]= HuffmanCompression(D);
[CR2, flag2]= ArithmeticCompression(D);
for i = 1 : nChannels
    msg = D(:,i);
    [CRH(i), flagH(i)]= HuffmanCompression(msg);
    [CRA(i), flagA(i)]= ArithmeticCompression(msg);
end
CR1
CR2
CRH_mean = mean(CRH)
CRA_mean = mean(CRA)
figure;
subplot(2,1,1);
histogram(A);
title('Raw histogram');
subplot(2,1,2);
histogram(D);
title('Histogram after delta-sampling');
% subplot(3,1,3);
% histogram(SD);
% title('Histogram after standardization and delta-sampling');
figure;
plot(1:nChannels,CRH,1:nChannels,CRA);
legend('Huffman (each channel)', 'Arithmetic (each channel)');
=======
clear all; close all; clc;
load('32ChannelEEG.mat');
% for i = 1:500
%     fprintf('%d, ',A(i,3));
% end
nSamples = 500;
nChannels = 32;

O = ones(nSamples, nChannels);
%S = zeros(nSamples, nChannels);
SD = zeros(nSamples, nChannels);

meanOfEachChannel = mean(A);
stdOfEachChannel = std(A);
S = (A - gmultiply(O,meanOfEachChannel))./gmultiply(O,stdOfEachChannel);

D(1 , :) = A(1 , :);
for i = 2 : nSamples
    D(i, :) = A(i, :) - A(i - 1, :);
end

SD(1 , :) = S(1 , :);
for i = 2 : nSamples
    SD(i, :) = S(i, :) - S(i - 1, :);
end

[CR1, flag1]= HuffmanCompression(D);
[CR2, flag2]= ArithmeticCompression(D);
for i = 1 : nChannels
    msg = D(:,i);
    [CRH(i), flagH(i)]= HuffmanCompression(msg);
    [CRA(i), flagA(i)]= ArithmeticCompression(msg);
end
CR1
CR2
CRH_mean = mean(CRH)
CRA_mean = mean(CRA)
figure;
subplot(2,1,1);
histogram(A);
title('Raw histogram');
subplot(2,1,2);
histogram(D);
title('Histogram after delta-sampling');
% subplot(3,1,3);
% histogram(SD);
% title('Histogram after standardization and delta-sampling');
figure;
plot(1:nChannels,CRH,1:nChannels,CRA);
legend('Huffman (each channel)', 'Arithmetic (each channel)');
>>>>>>> a8e4fc521e99e882afcff243c6c8f5bff6ecfcb0
grid on; grid minor;