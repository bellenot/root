// @(#)root/graf:$Id$
// Author: Rene Brun   08/08/2002

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TCrown
#define ROOT_TCrown


#include "TEllipse.h"


class TCrown : public TEllipse {
   Double_t fYXRatio = 1;  // ratio between Y and X axis

public:
   TCrown();
   TCrown(Double_t x1, Double_t y1,Double_t radin, Double_t radout,
          Double_t phimin=0,Double_t phimax=360);
   TCrown(const TCrown &crown);
   ~TCrown() override;

   void SetYXRatio(Double_t v = 1) { fYXRatio = v; }
   Double_t GetYXRatio() const { return fYXRatio; }

   void Copy(TObject &crown) const override;
   Int_t   DistancetoPrimitive(Int_t px, Int_t py) override;
   virtual TCrown *DrawCrown(Double_t x1, Double_t y1, Double_t radin, Double_t radout,
                             Double_t  phimin=0, Double_t  phimax=360, Option_t *option="");
   void    ExecuteEvent(Int_t event, Int_t px, Int_t py) override;
   Int_t   IsInside(Double_t x, Double_t y) const;
   void    Paint(Option_t *option="") override;
   void    SavePrimitive(std::ostream &out, Option_t *option = "") override;

   ClassDefOverride(TCrown,2)  //A crown or segment of crown
};

#endif
