#include "../include/Drawable.h"
#include "../include/PlotTools.h"
#include<iostream>
#include <THStack.h>
namespace Drawing {
Drawable1D  makeStack(std::vector<Drawable1D>& hists,Drawable1D& totStack) {
    Drawable1D newDrawable;
    newDrawable.type = STACK;
    newDrawable.drawOpt = "";
    newDrawable.title = "";
    THStack * stack = new THStack;
    for(auto& h : hists){
        stack->Add((TH1*)h.obj,"HIST");
        newDrawable.stackTitles.push_back(h.title);
    }
    newDrawable.obj = stack;
    newDrawable.totStack = (TH1*)totStack.obj;
    return newDrawable;
}

Drawable1D  makeStack(std::vector<TH1*>& hists,std::vector<TString>& titles, TH1 * totStack){
    std::vector<Drawable1D> histDs;
    for(unsigned int iH = 0; iH < hists.size(); ++iH){
        histDs.emplace_back("",titles[iH],Drawing::HIST1D,hists[iH],false);
    }
    Drawable1D totD;
    totD.obj = totStack;
    totD.type = Drawing::HIST1D;
    return makeStack(histDs,totD);
}

Drawable1D  convertToPoisson(Drawable1D& input, bool drawTrailingZeros) {
    Drawable1D newDrawable;
    if(input.type != HIST1D) return input;
    if(!input.doPoisson) return input;
    TH1 * hist = (TH1*)input.obj;
    auto * graph = PlotTools::getPoissonErrors(hist,drawTrailingZeros);
    graph->SetLineColor  (hist->GetLineColor());
    graph->SetLineWidth  (hist->GetLineWidth());
    graph->SetLineStyle  (hist->GetLineStyle());
    graph->SetMarkerStyle(hist->GetMarkerStyle());
    graph->SetMarkerColor(hist->GetMarkerColor());
    graph->SetMarkerSize (hist->GetMarkerSize());
    newDrawable.type = GRAPH;
    newDrawable.drawOpt = input.drawOpt.Contains("P") ? "P" : "";
    if(input.drawOpt.Contains("E")) newDrawable.drawOpt  += "0";
    newDrawable.obj = graph;
    newDrawable.graphAxisHist = (TH1*)hist->Clone();
    newDrawable.title = input.title;
    return newDrawable;

}
Drawable1D  makeHist(TH1* hist, TString title, TString drawOpt,
        bool doPoissonErrors,const bool drawTrailingPoissonZeros){
    Drawable1D newDrawable(drawOpt,title,Drawing::HIST1D,hist,doPoissonErrors);
    if(doPoissonErrors){
        return convertToPoisson(newDrawable,drawTrailingPoissonZeros);
    } else {
        return newDrawable;
    }
}
Drawable1D  makeRatio(const Drawable1D num, const TH1* den, bool doBinomErrors,
        const bool drawTrailingPoissonZeros) {
    Drawable1D newDrawable;
    if(num.type == STACK || num.type == NONE) {
        newDrawable = num;
        newDrawable.obj = newDrawable.obj->Clone();
    }
    else if(num.type == GRAPH) {
        auto gr = (TGraphAsymmErrors*) num.obj->Clone();
        std::vector<int> zerPts;
        for(int iP = 0; iP < gr->GetN();++iP){

            double x,y;
            gr->GetPoint(iP,x,y);
            double el = gr->GetErrorYlow(iP);
            double eh = gr->GetErrorYhigh(iP);
            int denBin = den->FindFixBin(x);
            double deny = den->GetBinContent(denBin);
            double denErr = den->GetBinError(denBin);
            if(deny){
                gr->SetPoint(iP,x,y/deny);
                if(y) el = (y/deny)*std::sqrt(el*el/(y*y) + denErr*denErr/(deny*deny) );
                if(y) eh = (y/deny)*std::sqrt(eh*eh/(y*y) + denErr*denErr/(deny*deny) );
                gr->SetPointEYhigh(iP,eh);
                gr->SetPointEYlow(iP,el);
            } else {
                zerPts.push_back(iP);
            }
        }
        for(unsigned int iP = zerPts.size(); iP >0; --iP ){
            gr->RemovePoint(zerPts[iP-1]);
        }
        newDrawable.type = GRAPH;
        newDrawable.drawOpt = num.drawOpt;
        newDrawable.obj = gr;
        newDrawable.graphAxisHist = (TH1*)den->Clone(TString::Format("%s_grcp",gr->GetName()));

    } else if(num.type==HIST1D){
        TH1* hist = (TH1*)num.obj->Clone();
        if(num.drawOpt.Contains("E") && (num.doPoisson || doBinomErrors)){
            auto * graph = doBinomErrors ? PlotTools::getBinomErrors(hist,den) :
                    PlotTools::getRatioPoissonErrors(hist,den,drawTrailingPoissonZeros);
            graph->SetLineColor  (hist->GetLineColor());
            graph->SetLineWidth  (hist->GetLineWidth());
            graph->SetLineStyle  (hist->GetLineStyle());
            graph->SetMarkerStyle(hist->GetMarkerStyle());
            graph->SetMarkerColor(hist->GetMarkerColor());
            graph->SetMarkerSize (hist->GetMarkerSize());
            newDrawable.type = GRAPH;
            newDrawable.drawOpt = num.drawOpt.Contains("P") ? "P" : "";
            if(num.drawOpt.Contains("E")) newDrawable.drawOpt  += "0";
            newDrawable.obj = graph;
            newDrawable.graphAxisHist = hist;
        } else {
            hist->Divide(den);
            newDrawable.type = HIST1D;
            newDrawable.drawOpt = num.drawOpt;
            newDrawable.obj = hist;
        }
    }
    newDrawable.title = num.title;
    return newDrawable;
}


Drawable1D  makeRatio(TH1* num, const TH1 * den, TString title, TString drawOpt,
        bool doBinomErrors, bool doPoissonErrors, const bool drawTrailingPoissonZeros){
    const Drawable1D newDrawable (drawOpt,title,HIST1D,num,doPoissonErrors);
    return makeRatio(newDrawable,den,doBinomErrors,drawTrailingPoissonZeros);
}

double getMax(const Drawable1D& drawable){

    if(drawable.type == STACK || drawable.type == HIST1D){
        const TH1 * hist = drawable.type == STACK  ? drawable.totStack : (const TH1*)drawable.obj;
        double max = 0;
        for(int iB = 0; iB <= hist->GetNbinsX()+1; ++iB) max = std::max(hist->GetBinContent(iB) + hist->GetBinError(iB), max );
        return max;
    }

    double max = 0;
    double x, y;
    const TGraphAsymmErrors * graph = (const TGraphAsymmErrors *)drawable.obj;
    for(int iB = 0; iB < graph->GetN(); ++iB){
        graph->GetPoint(iB,x,y);
        //        std::cout << iB << " "<< y <<" "<<graph->GetErrorYhigh(iB)<<std::endl;
        max = std::max(y + graph->GetErrorYhigh(iB), max );
    }
    return max;
}

}


