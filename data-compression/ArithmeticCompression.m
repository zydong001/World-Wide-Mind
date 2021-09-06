function [cr,flag] = ArithmeticCompression(msg)
[M,N] = size(msg);
data_row = reshape(msg,M*N,1);  
[alphabet, ~, seq] = unique(data_row);
counts = histc(data_row,alphabet);

code = arithenco(seq, counts);
dseq=arithdeco(code,counts,length(data_row));
dec_data_row=zeros(length(dseq),1);
for i=1:length(dseq)
    a=dseq(i);
    dec_data_row(i)=alphabet(a);
end
decMsg = reshape(dec_data_row, M, N);
cr = length(data_row)*2 / (length(counts)*2 + length(code)/8);
flag = isequal(msg,decMsg);