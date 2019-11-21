
export interface Sample {
    voltage: number;
    current: number;
}

export interface SamplesPackage {
    time: number;
    voltageScaleFactor: number;
    currentScaleFactor: number;
    samples: Sample[];
}