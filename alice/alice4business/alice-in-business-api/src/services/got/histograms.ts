import { makeInitialTimeHistogram, Histogram, fillHist } from '../solomon';

export const getHistogramName = (sourceName: string) =>
    `requests_${sourceName}_time_hgram`;

export class Histograms {
    private intHistograms: Histogram = {};

    public createHistogram = (sourceName: string) => {
        const histogramName = getHistogramName(sourceName);

        if (!this.intHistograms[histogramName]) {
            this.intHistograms[histogramName] = makeInitialTimeHistogram();
        }
    };

    public writeHistogramValue = (sourceName: string, ms: number) => {
        const histogramName = getHistogramName(sourceName);

        if (this.intHistograms[histogramName]) {
            fillHist(this.intHistograms[histogramName], ms);
        }
    };

    public get histograms() {
        return this.intHistograms;
    }
}

const gotHistograms = new Histograms();

export default gotHistograms;
