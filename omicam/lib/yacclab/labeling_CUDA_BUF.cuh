#pragma once
class BUF_IC : public GpuLabeling2D<CONN_8> {
private:
dim3 grid_size_;
dim3 block_size_;

public:
BUF_IC() {}

void PerformLabeling();


private:
void Alloc();

void Dealloc();

double MemoryTransferHostToDevice();

void MemoryTransferDeviceToHost();

void AllScans();

public:
void PerformLabelingWithSteps();

};