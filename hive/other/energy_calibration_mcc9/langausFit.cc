#include "energy_cal.h"
#include "LandauGaussian.h"
using std::cout;
using std::endl;
std::pair<double,double> getFitRange(TH1D*);

void langausFit() {
    TString tag = "all";
    TFile *fin = new TFile("output_energy_correction_"+tag+".root", "READ");
    if (!fin) {
        cout << "Bad input file" << endl;
        return;
    }

    TF1 *langaus = new TF1("langaus", landauGaussian, 0, 0.5, 4);
    langaus->SetParNames("Landau width","Peak value","Normalization","Gaussian width");

    std::vector< std::pair<int,int> > bins = leadingProjBins();
    TCanvas *c = new TCanvas("c", "c", 1900, 1000);
    // TODO WARNING Hard-coding alert
    c->Divide(4, 5 );
    gStyle->SetOptStat(0);
    gStyle->SetOptFit(1);
    gStyle->SetTextSize(2.);
    gROOT->ForceStyle();

    TString histName;
    std::pair<double,double> fitRange;
    TFile *fout = new TFile("output_langaus_fits.root", "RECREATE");
    std::vector<double> peakVals    (bins.size() );
    std::vector<double> peakValsErrs(bins.size() );
    std::ofstream of;
    of.open("peakvals.dat");

    for (int i = 0; i < bins.size(); i++) {
        c->cd(i+1);
        histName = Form("h%i", i+1);
        TH1D *h = (TH1D*)fin->Get(histName);
        if (!h) {
            cout << "Bad histogram" << endl;
            return;
        }
        //if (i+1 > 4) h->Rebin(2); // Rebin low-stats histos
        fitRange = getFitRange(h);
        cout << "Fit from " << fitRange.first << " to " << fitRange.second << endl;
        int    maxBin   = h->GetMaximumBin();
        double peakVal  = h->GetXaxis()->GetBinCenter(maxBin);
        int    nentries = h->GetEntries();
        cout << "Entries: " << nentries << endl;

        //langaus->SetParameters(0.004, peakVal, nentries, 0.005);
        double width = h->GetStdDev();
        /*
        if (i+1 <= 4) {
            langaus->SetParameters(0.004, peakVal, 0.8*nentries, 0.005);
            h->Fit(langaus,"q", "", 0.5*peakVal, 2.*peakVal);
        }
        else if (i+1 > 4 && i < 6) {
            langaus->SetParameters(0.008, peakVal, 0.7*nentries, 0.01);
            h->Fit(langaus,"q", "", 0.5*peakVal, 1.5*peakVal);
        }
        else {
            langaus->SetParameters(0.011, peakVal, 0.4*nentries, 0.015);
            h->Fit(langaus,"q", "", 0.5*peakVal, 1.2*peakVal);
        }
        */
        //langaus->SetParameters(0.004, peakVal, nentries, 0.005);
        langaus->SetParameters(0.001, peakVal, h->Integral(fitRange.first, fitRange.second, ""), 0.001);
        h->Fit(langaus,"q", "", fitRange.first, fitRange.second);
        h->GetXaxis()->SetTitleOffset(1.2);
        h->SetTitle("");
        h->Draw("ep");

        peakVals.at(i) = langaus->GetParameter(1); // Get peak value from fit
        cout << "Filling peak val " << langaus->GetParameter(1) << endl;
        of << peakVals.at(i) << endl;

    }
    c->SaveAs("fits_langaus.png", "png");
}

int main() {
    langausFit();
    return 0;
}

std::pair<double, double> getFitRange(TH1D *h1) {
    int maxBin = h1->GetMaximumBin();
    double peakVal = h1->GetXaxis()->GetBinCenter(maxBin);
    cout << "Peak val = " << peakVal << endl;
    std::pair<double, double> range = std::make_pair(peakVal*0.5, peakVal*1.5);

    return range;
}



