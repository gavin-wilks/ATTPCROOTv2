// FAIRROOT classes
#include "FairRootManager.h"
#include "FairRun.h"
#include "FairRuntimeDb.h"

#include <iostream>
#include <memory>

#include "ATAnalysisTask.hh"
#include "ATProtoAnalysis.hh"

ClassImp(ATAnalysisTask);

ATAnalysisTask::ATAnalysisTask()
{
  fLogger = FairLogger::GetLogger();
  fIsPersistence         = kFALSE;
  fIsPhiReco             = kFALSE;
  fIsFullScale           = kFALSE;
  fHoughDist             = 2.0;
  fRunNum                = 0;
  fInternalID            = 0;
  fProtoEventAnaArray    = new TClonesArray("ATProtoEventAna");
  fTrackingEventAnaArray = new TClonesArray("ATTrackingEventAna");
  fProtoAnalysis         = NULL;
  fTrackingAnalysis      = NULL;
}

ATAnalysisTask::~ATAnalysisTask()
{

   for (Int_t i=0;i<4;i++){
       delete fHoughFit[i];
       delete fHitPatternFilter[i];
       delete fFitResult[i];
    }

}

void ATAnalysisTask::SetPersistence(Bool_t value)           { fIsPersistence = value; }
void ATAnalysisTask::SetPhiReco()                           { fIsPhiReco = kTRUE;}
void ATAnalysisTask::SetFullScale()                         { fIsFullScale = kTRUE;}
void ATAnalysisTask::SetHoughDist(Double_t value)           { fHoughDist = value;}
void ATAnalysisTask::SetUpperLimit(Double_t value)          { fUpperLimit=value;}
void ATAnalysisTask::SetLowerLimit(Double_t value)          { fLowerLimit=value;}


InitStatus
ATAnalysisTask::Init()
{

  FairRootManager *ioMan = FairRootManager::Instance();
  if (ioMan == 0) {
    fLogger -> Error(MESSAGE_ORIGIN, "Cannot find RootManager!");
    return kERROR;
  }

  /*fEventHArray = (TClonesArray *) ioMan -> GetObject("ATEventH");
  if (fEventHArray == 0) {
    fLogger -> Error(MESSAGE_ORIGIN, "Cannot find ATEvent array!");
    return kERROR;
  }*/

  // ATANALYSIS IS BASED ON PATTERN RECOGNITION ALGORITHMS

  fHoughArray = (TClonesArray *) ioMan -> GetObject("ATHough");
  if (fHoughArray == 0) {
    fLogger -> Error(MESSAGE_ORIGIN, "Cannot find ATHough array!");
    //return kERROR;
  }else fLogger -> Error(MESSAGE_ORIGIN, "ATHough array found!");


  fRansacArray = (TClonesArray*) ioMan->GetObject("ATRansac");
  if (fRansacArray == 0) {
    fLogger -> Error(MESSAGE_ORIGIN, "Cannot find ATRansac array!");
    //return kERROR;
  }else fLogger -> Error(MESSAGE_ORIGIN, "ATRansac array found!");



  if(fIsPhiReco && fHoughArray){ //Find the Array of ProtoEvents

    fProtoAnalysis = new ATProtoAnalysis();
    fProtoAnalysis->SetHoughDist(fHoughDist);
    fProtoAnalysis->SetUpperLimit(fUpperLimit);
    fProtoAnalysis->SetLowerLimit(fLowerLimit);


      fProtoEventHArray = (TClonesArray *) ioMan -> GetObject("ATProtoEvent");
      if (fProtoEventHArray == 0) {
        fLogger -> Error(MESSAGE_ORIGIN, "Cannot find ATProtoEvent array! If SetPhiReco method is enabled, Phi Reconstruction is needed");
        return kERROR;
      }

      // For fitting the Prototype data
      for (Int_t i=0;i<4;i++){
        fHoughFit[i] =new TF1(Form("HoughFit%i",i)," (  (-TMath::Cos([0])/TMath::Sin([0]))*x ) + [1]/TMath::Sin([0])",0,120);
        fHitPatternFilter[i] = new TGraph();

      }

      ioMan -> Register("ATProtoEventAna", "ATTPC", fProtoEventAnaArray, fIsPersistence);

  }else if(fIsFullScale && fRansacArray){

      fTrackingAnalysis = new ATTrackingAnalysis();

      ioMan -> Register("ATTrackingEventAna", "ATTPC", fTrackingEventAnaArray, fIsPersistence);

  }



    return kSUCCESS;

}

void
ATAnalysisTask::SetParContainers()
{

  FairRun *run = FairRun::Instance();
  if (!run)
    fLogger -> Fatal(MESSAGE_ORIGIN, "No analysis run!");

  FairRuntimeDb *db = run -> GetRuntimeDb();
  if (!db)
    fLogger -> Fatal(MESSAGE_ORIGIN, "No runtime database!");

  fPar = (ATDigiPar *) db -> getContainer("ATDigiPar");
  if (!fPar)
    fLogger -> Fatal(MESSAGE_ORIGIN, "ATDigiPar not found!!");
}


void
ATAnalysisTask::Exec(Option_t *opt)
{

   fProtoEventAnaArray->Delete();
   fTrackingEventAnaArray->Delete();


   if(fIsPhiReco && fHoughArray){

          if (fProtoEventHArray -> GetEntriesFast() == 0)
            return;

          if (fHoughArray -> GetEntriesFast() == 0)
            return;


            // TODO:Use dynamic casting for each detector. Do the same in the Hough Task
            fHoughSpace  = (ATHoughSpaceLine *) fHoughArray -> At(0);
            if(fIsPhiReco) fProtoevent = (ATProtoEvent *) fProtoEventHArray -> At(0);
            fInternalID++;
            //std::cout << "  -I- ATAnalysisTask -  Event Number by Internal ID : "<<fInternalID<< std::endl;

            ATProtoEventAna *protoeventAna = (ATProtoEventAna *) new ((*fProtoEventAnaArray)[0]) ATProtoEventAna();

            // new ((*fAnalysisArray)[0]) ATProtoAnalysis();

            //ATProtoAnalysis * ProtoAnalysis = (ATProtoAnalysis *) new ((*fAnalysisArray)[0]) ATProtoAnalysis();
            //fProtoAnalysis = (ATProtoAnalysis *) fAnalysisArray->ConstructedAt(0);
            //std::auto_ptr<ATProtoAnalysis> ProtoAnalysis(new ATProtoAnalysis());
            fProtoAnalysis->Analyze(fProtoevent,protoeventAna,fHoughSpace,fHoughFit,fHitPatternFilter,fFitResult);

            for (Int_t i=0;i<4;i++){
              //fHoughFit[i]->Set(0);
              fHitPatternFilter[i]->Set(0);
              //fFitResult[i]->Clear(0);
            }

  }


}
