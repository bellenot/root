/// \file
/// \ingroup tutorial_ml
/// \notebook -nodraw
/// This macro provides examples for the training and testing of the
/// TMVA classifiers in categorisation mode.
/// - Project   : TMVA - a Root-integrated toolkit for multivariate data analysis
/// - Package   : TMVA
/// - Root Macro: TMVAClassificationCategory
///
/// As input data is used a toy-MC sample consisting of four Gaussian-distributed
/// and linearly correlated input variables with category (eta) dependent
/// properties.
///
/// For this example, only Fisher and Likelihood are used. Run via:
///
///     root -l TMVAClassificationCategory.C
///
/// The output file "TMVACC.root" can be analysed with the use of dedicated
/// macros (simply say: root -l <macro.C>), which can be conveniently
/// invoked through a GUI that will appear at the end of the run of this macro.
///
/// \macro_code
/// \macro_output
/// \author Andreas Hoecker


#include <cstdlib>
#include <iostream>
#include <map>
#include <string>

#include "TChain.h"
#include "TFile.h"
#include "TTree.h"
#include "TString.h"
#include "TObjString.h"
#include "TSystem.h"
#include "TROOT.h"

#include "TMVA/MethodCategory.h"
#include "TMVA/Factory.h"
#include "TMVA/DataLoader.h"
#include "TMVA/Tools.h"
#include "TMVA/TMVAGui.h"


// two types of category methods are implemented
Bool_t UseOffsetMethod = kTRUE;

void TMVAClassificationCategory()
{
   //---------------------------------------------------------------
   // Example for usage of different event categories with classifiers

   std::cout << std::endl << "==> Start TMVAClassificationCategory" << std::endl;

   // This loads the library
   TMVA::Tools::Instance();

   bool batchMode = false;

   // Create a new root output file.
   TString outfileName( "TMVACC.root" );
   TFile* outputFile = TFile::Open( outfileName, "RECREATE" );

   // Create the factory object (see TMVAClassification.C for more information)

  std::string factoryOptions( "!V:!Silent:Transformations=I;D;P;G,D" );
  if (batchMode) factoryOptions += ":!Color:!DrawProgressBar";

   TMVA::Factory *factory = new TMVA::Factory( "TMVAClassificationCategory", outputFile, factoryOptions );

   // Create DataLoader
   TMVA::DataLoader *dataloader=new TMVA::DataLoader("dataset");

   // Define the input variables used for the MVA training
   dataloader->AddVariable( "var1", 'F' );
   dataloader->AddVariable( "var2", 'F' );
   dataloader->AddVariable( "var3", 'F' );
   dataloader->AddVariable( "var4", 'F' );

   // You can add so-called "Spectator variables", which are not used in the MVA training,
   // but will appear in the final "TestTree" produced by TMVA. This TestTree will contain the
   // input variables, the response values of all trained MVAs, and the spectator variables
   dataloader->AddSpectator( "eta" );

   // Load the signal and background event samples from ROOT trees
   TFile *input(0);
   TString fname = gSystem->GetDirName(__FILE__) + "/data/";
   if (gSystem->AccessPathName( fname + "toy_sigbkg_categ_offset.root")) {
      // if directory data not found try using tutorials dir
      fname = gROOT->GetTutorialDir() + "/machine_learning/data/";
   }
   if (UseOffsetMethod) fname += "toy_sigbkg_categ_offset.root";
   else                 fname += "toy_sigbkg_categ_varoff.root";
   if (!gSystem->AccessPathName( fname )) {
      // first we try to find tmva_example.root in the local directory
      std::cout << "--- TMVAClassificationCategory: Accessing " << fname << std::endl;
      input = TFile::Open( fname );
   }

   if (!input) {
      std::cout << "ERROR: could not open data file: " << fname << std::endl;
      exit(1);
   }

   TTree *signalTree     = (TTree*)input->Get("TreeS");
   TTree *background = (TTree*)input->Get("TreeB");

   // Global event weights per tree (see below for setting event-wise weights)
   Double_t signalWeight     = 1.0;
   Double_t backgroundWeight = 1.0;

   // You can add an arbitrary number of signal or background trees
   dataloader->AddSignalTree    ( signalTree,     signalWeight     );
   dataloader->AddBackgroundTree( background, backgroundWeight );

   // Apply additional cuts on the signal and background samples (can be different)
   TCut mycuts = ""; // for example: TCut mycuts = "abs(var1)<0.5 && abs(var2-0.5)<1";
   TCut mycutb = ""; // for example: TCut mycutb = "abs(var1)<0.5";

   // Tell the factory how to use the training and testing events
   dataloader->PrepareTrainingAndTestTree( mycuts, mycutb,
                                        "nTrain_Signal=0:nTrain_Background=0:SplitMode=Random:NormMode=NumEvents:!V" );

   // Book MVA methods

   // Fisher discriminant
   factory->BookMethod( dataloader, TMVA::Types::kFisher, "Fisher", "!H:!V:Fisher" );

   // Likelihood
   factory->BookMethod( dataloader, TMVA::Types::kLikelihood, "Likelihood",
                        "!H:!V:TransformOutput:PDFInterpol=Spline2:NSmoothSig[0]=20:NSmoothBkg[0]=20:NSmoothBkg[1]=10:NSmooth=1:NAvEvtPerBin=50" );

   // Categorised classifier
   TMVA::MethodCategory* mcat = 0;

   // The variable sets
   TString theCat1Vars = "var1:var2:var3:var4";
   TString theCat2Vars = (UseOffsetMethod ? "var1:var2:var3:var4" : "var1:var2:var3");

   // Fisher with categories
   TMVA::MethodBase* fiCat = factory->BookMethod( dataloader, TMVA::Types::kCategory, "FisherCat","" );
   mcat = dynamic_cast<TMVA::MethodCategory*>(fiCat);
   mcat->AddMethod( "abs(eta)<=1.3", theCat1Vars, TMVA::Types::kFisher, "Category_Fisher_1","!H:!V:Fisher" );
   mcat->AddMethod( "abs(eta)>1.3",  theCat2Vars, TMVA::Types::kFisher, "Category_Fisher_2","!H:!V:Fisher" );

   // Likelihood with categories
   TMVA::MethodBase* liCat = factory->BookMethod( dataloader, TMVA::Types::kCategory, "LikelihoodCat","" );
   mcat = dynamic_cast<TMVA::MethodCategory*>(liCat);
   mcat->AddMethod( "abs(eta)<=1.3",theCat1Vars, TMVA::Types::kLikelihood,
                    "Category_Likelihood_1","!H:!V:TransformOutput:PDFInterpol=Spline2:NSmoothSig[0]=20:NSmoothBkg[0]=20:NSmoothBkg[1]=10:NSmooth=1:NAvEvtPerBin=50" );
   mcat->AddMethod( "abs(eta)>1.3", theCat2Vars, TMVA::Types::kLikelihood,
                    "Category_Likelihood_2","!H:!V:TransformOutput:PDFInterpol=Spline2:NSmoothSig[0]=20:NSmoothBkg[0]=20:NSmoothBkg[1]=10:NSmooth=1:NAvEvtPerBin=50" );

   // Now you can tell the factory to train, test, and evaluate the MVAs

   // Train MVAs using the set of training events
   factory->TrainAllMethods();

   // Evaluate all MVAs using the set of test events
   factory->TestAllMethods();

   // Evaluate and compare performance of all configured MVAs
   factory->EvaluateAllMethods();

   // --------------------------------------------------------------

   // Save the output
   outputFile->Close();

   std::cout << "==> Wrote root file: " << outputFile->GetName() << std::endl;
   std::cout << "==> TMVAClassificationCategory is done!" << std::endl;

   // Clean up
   delete factory;
   delete dataloader;

   // Launch the GUI for the root macros
   if (!gROOT->IsBatch()) TMVA::TMVAGui( outfileName );
}
int main( int argc, char** argv )
{
   TMVAClassificationCategory();
   return 0;
}
