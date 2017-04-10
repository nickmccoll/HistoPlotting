#include "THStack.h"
#include "../include/Plotter.h"
#include "../include/StyleInfo.h"
#include "../include/PlotTools.h"
void Plotter::addDrawable(Drawing::Drawable1D& input) {
	Drawing::Drawable1D newDrawable = input;
	newDrawable.obj = newDrawable.obj->Clone();
	hists.push_back(newDrawable);
}
TH1* Plotter::addHist(const TH1 * hist, TString title, int lineColor, int lineStyle, int lineWidth, int markerStyle, int markerSize, bool drawMarker, bool drawErrorBars, bool poissonErrors, TString drawOption){
  TH1* h = (TH1*)hist->Clone();
  //style
  h->SetLineColor(lineColor >= 0 ? lineColor : StyleInfo::getLineColor(hists.size()) );
  h->SetLineWidth(lineWidth);

  if(drawMarker){
    h->SetMarkerColor(lineColor >= 0 ? lineColor : StyleInfo::getLineColor(hists.size()) );
    h->SetMarkerStyle(markerStyle);
    h->SetMarkerSize(markerSize);
  } else {
    h->SetLineStyle(lineStyle);
    h->SetMarkerStyle(0);
    h->SetMarkerSize(0);
  }
  if(drawOption == ""){
    if(drawErrorBars) drawOption = "E1X0 ";
    if(drawMarker) drawOption += "P ";
  }
  hists.emplace_back(drawOption,title,Drawing::HIST1D,h,poissonErrors);
  return h;
}

TH1* Plotter::addStackHist(const TH1 * hist, TString title, int fillColor, int fillStyle, int lineColor, int lineWidth){
  TH1* h = (TH1*)hist->Clone();
  if(totStack.type == Drawing::NONE){
	  totStack.obj = (TH1*)h->Clone();
	  totStack.type = Drawing::HIST1D;
  } else {
	  ((TH1*)totStack.obj)->Add(h);
  }
  //style
  h->SetLineWidth(lineWidth);
  h->SetLineColor(lineColor);
  h->SetMarkerStyle(0);
  h->SetMarkerSize(0);
  h->SetFillColor(fillColor >= 0 ? fillColor : StyleInfo::getFillColor(stackHists.size()));
  h->SetFillStyle(fillStyle);

  stackHists.emplace_back("",title,Drawing::HIST1D,h,false);
  return h;
}

TCanvas * Plotter::draw(bool save, TString printName){
	if(hists.size() == 0 && stackHists.size() == 0) return 0;

	TCanvas * c =new TCanvas(printName,printName);
	std::vector<Drawing::Drawable1D> drawables;
	prepHist(drawables);
	Drawing::drawPane(c,drawables,&topStyle,true);
	if(stackHists.size()){
		topStyle.yAxis->SetTitle(topStyle.yTitle == "DEF" ? stackHists[0].getYTitle() : topStyle.yTitle.Data());
		topStyle.xAxis->SetTitle(topStyle.xTitle == "DEF" ? stackHists[0].getXTitle() : topStyle.xTitle.Data());
	} else {
		topStyle.yAxis->SetTitle(topStyle.yTitle == "DEF" ? hists[0].getYTitle() : topStyle.yTitle.Data());
		topStyle.xAxis->SetTitle(topStyle.xTitle == "DEF" ? hists[0].getXTitle() : topStyle.xTitle.Data());
	}
	if(save) c->Print(printName);
	return c;
}

TCanvas * Plotter::drawRatio(int denIDX, TString stackTitle,bool doBinomErrors, bool save, TString printName){
  if(hists.size() == 0 && stackHists.size() == 0)throw std::invalid_argument("Plotter::drawRatio -> Need to provide histograms");;

  std::vector<Drawing::Drawable1D> ratDrawables;
  TString denTitle = prepRat(ratDrawables,denIDX,stackTitle,doBinomErrors);
  TCanvas * c =new TCanvas(printName,printName);
  Drawing::drawPane(c,ratDrawables,&topStyle,true);

  topStyle.yAxis->SetTitle(topStyle.yTitle == "DEF" ? TString::Format("N/N(%s)",denTitle.Data()).Data() : topStyle.yTitle.Data());
  if(stackHists.size()){
    topStyle.xAxis->SetTitle(topStyle.xTitle == "DEF" ? stackHists[0].getXTitle() : topStyle.xTitle.Data());
  } else {
    topStyle.xAxis->SetTitle(topStyle.xTitle == "DEF" ? hists[0].getXTitle() : topStyle.xTitle.Data());
  }

  if(save) c->Write(printName);
  return c;
}

TCanvas * Plotter::drawSplitRatio(int denIDX, TString stackTitle,bool doBinomErrors, bool save, TString printName){
  if(hists.size() == 0 && stackHists.size() == 0) return 0;

  double topScale = (.75)*(1/.66);
  if(topStyle.leg_y1 < 0) topStyle.leg_y1 *= (1/.66);
  topStyle.leg_y2 -= .05;


  TCanvas * c =new TCanvas(printName,printName);
  TPad *pad1 = new TPad("pad1", "pad1", 0, 0.33, 1, 1.0);
  pad1->SetBottomMargin(.025); // Upper and lower plot are joined
  pad1->SetTopMargin(.1);
  pad1->SetLeftMargin(.14);
  pad1->Draw();             // Draw the upper pad: pad1
  pad1->cd();               // pad1 becomes the current pad
  std::vector<Drawing::Drawable1D> drawables;
  prepHist(drawables);
  Drawing::drawPane(pad1,drawables,&topStyle,true);
  topStyle.xAxis->SetLabelOffset(2);


  topStyle.yAxis->SetLabelSize(topStyle.yAxis->GetLabelSize()*topScale );
  topStyle.yAxis->SetTitleOffset(topStyle.yAxis->GetTitleOffset()*.70);
  topStyle.yAxis->SetTitleSize(topStyle.yAxis->GetTitleSize()*topScale);

  if(stackHists.size()){
    topStyle.yAxis->SetTitle(topStyle.yTitle == "DEF" ? stackHists[0].getYTitle() : topStyle.yTitle.Data());
  } else {
    topStyle.yAxis->SetTitle(topStyle.yTitle == "DEF" ? hists[0].getYTitle() : topStyle.yTitle.Data());
  }

  c->cd();
  TPad *pad2 = new TPad("pad2", "pad2", 0, 0, 1, 0.33);
  pad2->SetTopMargin(.005);
  pad2->SetLeftMargin(.14);
  pad2->SetBottomMargin(.3);
  pad2->Draw();             // Draw the upper pad: pad1
  pad2->cd();               // pad1 becomes the current pad


  std::vector<Drawing::Drawable1D> ratDrawables;
  TString denTitle = prepRat(ratDrawables,denIDX,stackTitle,doBinomErrors);
  Drawing::drawPane(pad2,ratDrawables,&botStyle,false);
  double botScale = (.75)*(1/.33);
  botStyle.yAxis->SetLabelSize(botStyle.yAxis->GetLabelSize()*botScale);
  botStyle.yAxis->SetTitleSize(botStyle.yAxis->GetTitleSize()*botScale);
  botStyle.yAxis->SetTitleOffset(botStyle.yAxis->GetTitleOffset()*.33);
  botStyle.xAxis->SetLabelSize(botStyle.xAxis->GetLabelSize()*botScale);
  botStyle.xAxis->SetTitleSize(botStyle.xAxis->GetTitleSize()*botScale);

  botStyle.yAxis->SetTitle(topStyle.yTitle == "DEF" ? TString::Format("N/N(%s)",denTitle.Data()).Data() : botStyle.yTitle.Data());
  if(stackHists.size()){
    botStyle.xAxis->SetTitle(topStyle.xTitle == "DEF" ? stackHists[0].getXTitle() : botStyle.xTitle.Data());
  } else {
    botStyle.xAxis->SetTitle(topStyle.xTitle == "DEF" ? hists[0].getXTitle()  : botStyle.xTitle.Data());
  }

  if(save) c->Write(printName);
  return c;
}



void Plotter::scale(double scale){
  for(auto& h : hists) if(h.type == Drawing::HIST1D) ((TH1*)h.obj)->Scale(scale);
  for(auto& h : stackHists) if(h.type == Drawing::HIST1D) ((TH1*)h.obj)->Scale(scale);
  if(totStack.type != Drawing::NONE) ((TH1*)totStack.obj)->Scale(scale);
}
void Plotter::normalize() {
  for(auto& h : hists) if(h.type == Drawing::HIST1D) PlotTools::normalize((TH1*)h.obj);
  if(totStack.type != Drawing::NONE){
    double stackNorm = ((TH1*)totStack.obj)->Integral(0,-1);
    for(auto& h : stackHists) ((TH1*)h.obj)->Scale(1/stackNorm);
    ((TH1*)totStack.obj)->Scale(1/stackNorm);
  }
}

void Plotter::rebin(int n){
  for(auto& h : hists) if(h.type == Drawing::HIST1D) PlotTools::rebin(((TH1*)h.obj),n);
  for(auto& h : stackHists) if(h.type == Drawing::HIST1D) PlotTools::rebin(((TH1*)h.obj),n);
}
void Plotter::rebin(int n, double * bins){
  for(auto& h : hists) if(h.type == Drawing::HIST1D) PlotTools::rebin(((TH1*)h.obj),n,bins);
  for(auto& h : stackHists) if(h.type == Drawing::HIST1D) PlotTools::rebin(((TH1*)h.obj),n,bins);
}

void Plotter::prepHist(std::vector<Drawing::Drawable1D>& drawables){
	if(doUnderflow){
		for(auto& h : hists) if(h.type == Drawing::HIST1D) PlotTools::toUnderflow((TH1*)h.obj);
		for(auto& h : stackHists) if(h.type == Drawing::HIST1D) PlotTools::toUnderflow((TH1*)h.obj);
	}
	if(doOverflow){
		for(auto& h : hists) if(h.type == Drawing::HIST1D) PlotTools::toOverflow((TH1*)h.obj);
		for(auto& h : stackHists) if(h.type == Drawing::HIST1D) PlotTools::toOverflow((TH1*)h.obj);
	}
	if(stackHists.size())drawables.push_back(Drawing::makeStack(stackHists,totStack));
	if(hists.size())
		for(int iH = hists.size() -1; iH >= 0; --iH){
			if(hists[iH].doPoisson) drawables.push_back(Drawing::convertToPoisson(hists[iH]));
			else drawables.push_back(hists[iH]);
		}
}

TString Plotter::prepRat(std::vector<Drawing::Drawable1D>& drawables, int denIDX, TString stackTitle,bool doBinomErrors){
	if(doUnderflow){
		for(auto& h : hists) if(h.type == Drawing::HIST1D) PlotTools::toUnderflow((TH1*)h.obj);
		for(auto& h : stackHists) if(h.type == Drawing::HIST1D) PlotTools::toUnderflow((TH1*)h.obj);
	}
	if(doOverflow){
		for(auto& h : hists) if(h.type == Drawing::HIST1D) PlotTools::toOverflow((TH1*)h.obj);
		for(auto& h : stackHists) if(h.type == Drawing::HIST1D) PlotTools::toOverflow((TH1*)h.obj);
	}

	TH1 * den = 0;
	TString denTitle;
	if(denIDX < 0){
		den = (TH1*)totStack.obj;
		denTitle = stackTitle;
	}  else if(denIDX < int(hists.size())) {
		den = (TH1*)hists[denIDX].obj;
		denTitle = hists[denIDX].title;
	} else {throw std::invalid_argument("Plotter::prepRat -> Need to provide a denominator");;}

	if(denIDX >= 0 && totStack.obj) drawables.push_back(Drawing::makeRatio((TH1*)totStack.obj, den,stackTitle,"",doBinomErrors));
	if(hists.size())
		for(int iH = hists.size() -1; iH >= 0; --iH){
			if(int(iH) == denIDX) continue;
			drawables.push_back(Drawing::makeRatio(hists[iH], den,doBinomErrors));
		}
	return denTitle;
}
