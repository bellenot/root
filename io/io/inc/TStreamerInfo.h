// @(#)root/io:$Id$
// Author: Rene Brun   12/10/2000

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TStreamerInfo
#define ROOT_TStreamerInfo

#include <atomic>
#include <vector>

#include "TVirtualStreamerInfo.h"

#include "TVirtualCollectionProxy.h"

#include "TObjArray.h"


class TFile;
class TClass;
class TClonesArray;
class TDataMember;
class TMemberStreamer;
class TStreamerElement;
class TStreamerBasicType;
class TClassStreamer;
class TVirtualArray;
namespace ROOT { namespace Detail { class TCollectionProxyInfo; } }
namespace ROOT { class TSchemaRule; }

namespace TStreamerInfoActions { class TActionSequence; }

class TStreamerInfo : public TVirtualStreamerInfo {

   class TCompInfo {
   // Class used to cache information (see fComp)
   private:
      // TCompInfo(const TCompInfo&) = default;
      // TCompInfo& operator=(const TCompInfo&) = default;
   public:
      Int_t             fType;
      Int_t             fNewType;
      Int_t             fOffset;
      Int_t             fLength;
      TStreamerElement *fElem;     ///< Not Owned
      ULongptr_t        fMethod;
      TClass           *fClass;    ///< Not Owned
      TClass           *fNewClass; ///< Not Owned
      TString           fClassName;
      TMemberStreamer  *fStreamer; ///< Not Owned
      TCompInfo() : fType(-1), fNewType(0), fOffset(0), fLength(0), fElem(nullptr), fMethod(0),
                    fClass(nullptr), fNewClass(nullptr), fClassName(), fStreamer(nullptr) {}
      ~TCompInfo() {}
      void Update(const TClass *oldcl, TClass *newcl);
   };
   friend class TStreamerInfoActions::TActionSequence;

public:
   // make the opaque pointer public.
   typedef TCompInfo TCompInfo_t;

protected:
   //---------------------------------------------------------------------------
   // Adapter class used to handle streaming collection of pointers
   //---------------------------------------------------------------------------
   class TPointerCollectionAdapter
   {
   public:
      TPointerCollectionAdapter( TVirtualCollectionProxy *proxy ):
         fProxy( proxy ) {}

      char* operator[]( UInt_t idx ) const
      {
         char **el = (char**)fProxy->At(idx);
         return *el;
      }
   private:
      TVirtualCollectionProxy *fProxy;
   };

private:
   UInt_t            fCheckSum;          ///<Checksum of original class
   Int_t             fClassVersion;      ///<Class version identifier
   Int_t             fOnFileClassVersion;///<!Class version identifier as stored on file.
   Int_t             fNumber;            ///<!Unique identifier
   Int_t             fSize;              ///<!size of the persistent class
   Int_t             fNdata;             ///<!number of optimized elements
   Int_t             fNfulldata;         ///<!number of elements
   Int_t             fNslots;            ///<!total number of slots in fComp.
   TCompInfo        *fComp;              ///<![fNslots with less than fElements->GetEntries()*1.5 used] Compiled info
   TCompInfo       **fCompOpt;           ///<![fNdata]
   TCompInfo       **fCompFull;          ///<![fElements->GetEntries()]
   TClass           *fClass;             ///<!pointer to class
   TObjArray        *fElements;          ///<Array of TStreamerElements
   Version_t         fOldVersion;        ///<! Version of the TStreamerInfo object read from the file
   Int_t             fNVirtualInfoLoc;   ///<! Number of virtual info location to update.
   ULong_t          *fVirtualInfoLoc;    ///<![fNVirtualInfoLoc] Location of the pointer to the TStreamerInfo inside the object (when emulated)
   TStreamerInfoActions::TActionSequence *fReadObjectWise;        ///<! List of read action resulting from the compilation.
   TStreamerInfoActions::TActionSequence *fReadMemberWise;        ///<! List of read action resulting from the compilation for use in member wise streaming.
   TStreamerInfoActions::TActionSequence *fReadMemberWiseVecPtr;  ///<! List of read action resulting from the compilation for use in member wise streaming.
   TStreamerInfoActions::TActionSequence *fReadText;              ///<! List of text read action resulting from the compilation, used for JSON.
   TStreamerInfoActions::TActionSequence *fWriteObjectWise;       ///<! List of write action resulting from the compilation.
   TStreamerInfoActions::TActionSequence *fWriteMemberWise;       ///<! List of write action resulting from the compilation for use in member wise streaming.
   TStreamerInfoActions::TActionSequence *fWriteMemberWiseVecPtr; ///<! List of write action resulting from the compilation for use in member wise streaming.
   TStreamerInfoActions::TActionSequence *fWriteText;             ///<! List of text write action resulting for the compilation, used for JSON.

   static std::atomic<Int_t>             fgCount;     ///<Number of TStreamerInfo instances

   template <typename T> static T GetTypedValueAux(Int_t type, void *ladd, int k, Int_t len);
   static void       PrintValueAux(char *ladd, Int_t atype, TStreamerElement * aElement, Int_t aleng, Int_t *count);

   UInt_t            GenerateIncludes(FILE *fp, char *inclist, const TList *extrainfos);
   void              GenerateDeclaration(FILE *fp, FILE *sfp, const TList *subClasses, Bool_t top = kTRUE);
   void              InsertArtificialElements(std::vector<const ROOT::TSchemaRule*> &rules);
   void              DestructorImpl(void* p, Bool_t dtorOnly);

private:
   TStreamerInfo(const TStreamerInfo&) = delete;            // TStreamerInfo are not copiable.  Not Implemented.
   TStreamerInfo& operator=(const TStreamerInfo&) = delete; // TStreamerInfo are not copiable.  Not Implemented.
   void AddReadAction(TStreamerInfoActions::TActionSequence *readSequence, Int_t index, TCompInfo *compinfo);
   void AddWriteAction(TStreamerInfoActions::TActionSequence *writeSequence, Int_t index, TCompInfo *compinfo);
   void AddReadTextAction(TStreamerInfoActions::TActionSequence *readSequence, Int_t index, TCompInfo *compinfo);
   void AddWriteTextAction(TStreamerInfoActions::TActionSequence *writeSequence, Int_t index, TCompInfo *compinfo);
   void AddReadMemberWiseVecPtrAction(TStreamerInfoActions::TActionSequence *readSequence, Int_t index, TCompInfo *compinfo);
   void AddWriteMemberWiseVecPtrAction(TStreamerInfoActions::TActionSequence *writeSequence, Int_t index, TCompInfo *compinfo);

public:

   /// Status bits
   /// See TVirtualStreamerInfo::EStatusBits for the values.

   /// EReadWrite Enumerator
   /// See TVirtualStreamerInfo::EReadWrite for documentation and values.

   TStreamerInfo();
   TStreamerInfo(TClass *cl);
              ~TStreamerInfo() override;
   void                Build(Bool_t isTransient = kFALSE) override;
   void                BuildCheck(TFile *file = nullptr, Bool_t load = kTRUE) override;
   void                BuildEmulated(TFile *file) override;
   void                BuildOld() override;
   Bool_t              BuildFor( const TClass *cl ) override;
   void                CallShowMembers(const void* obj, TMemberInspector &insp, Bool_t isTransient) const override;
   void                Clear(Option_t * = "") override;
   TObject            *Clone(const char *newname = "") const override;
   Bool_t              CompareContent(TClass *cl,TVirtualStreamerInfo *info, Bool_t warn, Bool_t complete, TFile *file) override;
   void                Compile() override;
   void                ComputeSize();
   void                ForceWriteInfo(TFile *file, Bool_t force = kFALSE) override;
   Int_t               GenerateHeaderFile(const char *dirname, const TList *subClasses = nullptr, const TList *extrainfos = nullptr) override;
   TClass             *GetActualClass(const void *obj) const override;
   TClass             *GetClass() const override { return fClass; }
   UInt_t              GetCheckSum() const override { return fCheckSum; }
   UInt_t              GetCheckSum(TClass::ECheckSum code) const;
   Int_t               GetClassVersion() const override { return fClassVersion; }
   Int_t               GetDataMemberOffset(TDataMember *dm, TMemberStreamer *&streamer) const;
   TObjArray          *GetElements() const override {return fElements;}
   TStreamerElement   *GetElem(Int_t id) const override { return fComp[id].fElem; }  // Return the element for the list of optimized elements (max GetNdata())
   TStreamerElement   *GetElement(Int_t id) const override {return (TStreamerElement*)fElements->At(id);} // Return the element for the complete list of elements (max GetElements()->GetEntries())
   Int_t               GetElementOffset(Int_t id) const override {return fCompFull[id]->fOffset;}
   TStreamerInfoActions::TActionSequence *GetReadMemberWiseActions(Bool_t forCollection) { return forCollection ? fReadMemberWiseVecPtr : fReadMemberWise; }
   TStreamerInfoActions::TActionSequence *GetReadObjectWiseActions() { return fReadObjectWise; }
   TStreamerInfoActions::TActionSequence *GetReadTextActions() { return fReadText; }
   TStreamerInfoActions::TActionSequence *GetWriteMemberWiseActions(Bool_t forCollection) { return forCollection ? fWriteMemberWiseVecPtr : fWriteMemberWise; }
   TStreamerInfoActions::TActionSequence *GetWriteObjectWiseActions() { return fWriteObjectWise; }
   TStreamerInfoActions::TActionSequence *GetWriteTextActions() { return fWriteText; }
   Int_t               GetNdata()   const {return fNdata;}
   Int_t               GetNelement() const { return fElements->GetEntriesFast(); }
   Int_t               GetNumber()  const override { return fNumber; }
   Int_t               GetLength(Int_t id) const {return fComp[id].fLength;}
   ULongptr_t          GetMethod(Int_t id) const {return fComp[id].fMethod;}
   Int_t               GetNewType(Int_t id) const {return fComp[id].fNewType;}
   Int_t               GetOffset(const char *) const override;
   Int_t               GetOffset(Int_t id) const override {return fComp[id].fOffset;}
   Version_t           GetOldVersion() const override {return fOldVersion;}
   Int_t               GetOnFileClassVersion() const override {return fOnFileClassVersion;}
   Int_t               GetSize() const override;
   Int_t               GetSizeElements() const;
   TStreamerElement   *GetStreamerElement(const char*datamember, Int_t& offset) const  override;
   TStreamerElement   *GetStreamerElementReal(Int_t i, Int_t j) const;
   Int_t               GetType(Int_t id)   const {return fComp[id].fType;}
   template <typename T> T GetTypedValue(char *pointer, Int_t i, Int_t j, Int_t len) const;
   template <typename T> T GetTypedValueClones(TClonesArray *clones, Int_t i, Int_t j, Int_t k, Int_t eoffset) const;
   template <typename T> T GetTypedValueSTL(TVirtualCollectionProxy *cont, Int_t i, Int_t j, Int_t k, Int_t eoffset) const;
   template <typename T> T GetTypedValueSTLP(TVirtualCollectionProxy *cont, Int_t i, Int_t j, Int_t k, Int_t eoffset) const;
   Double_t            GetValue(char *pointer, Int_t i, Int_t j, Int_t len) const { return GetTypedValue<Double_t>(pointer, i, j, len); }
   Double_t            GetValueClones(TClonesArray *clones, Int_t i, Int_t j, Int_t k, Int_t eoffset) const { return GetTypedValueClones<Double_t>(clones, i, j, k, eoffset); }
   Double_t            GetValueSTL(TVirtualCollectionProxy *cont, Int_t i, Int_t j, Int_t k, Int_t eoffset) const { return GetTypedValueSTL<Double_t>(cont, i, j, k, eoffset); }
   Double_t            GetValueSTLP(TVirtualCollectionProxy *cont, Int_t i, Int_t j, Int_t k, Int_t eoffset) const { return GetTypedValueSTLP<Double_t>(cont, i, j, k, eoffset); }
   void                ls(Option_t *option="") const override;
   Bool_t              MatchLegacyCheckSum(UInt_t checksum) const;
   TVirtualStreamerInfo *NewInfo(TClass *cl) override { return new TStreamerInfo(cl); }
   void               *New(void *obj = nullptr) override;
   void               *NewArray(Long_t nElements, void* ary = nullptr) override;
   void                Destructor(void* p, Bool_t dtorOnly = kFALSE) override;
   void                DeleteArray(void* p, Bool_t dtorOnly = kFALSE) override;
   void                PrintValue(const char *name, char *pointer, Int_t i, Int_t len, Int_t lenmax=1000) const;
   void                PrintValueClones(const char *name, TClonesArray *clones, Int_t i, Int_t eoffset, Int_t lenmax=1000) const;
   void                PrintValueSTL(const char *name, TVirtualCollectionProxy *cont, Int_t i, Int_t eoffset, Int_t lenmax=1000) const;

   template <class T>
   Int_t               ReadBuffer(TBuffer &b, const T &arrptr, TCompInfo *const*const compinfo, Int_t first, Int_t last, Int_t narr=1,Int_t eoffset=0,Int_t mode=0);
   template <class T>
   Int_t               ReadBufferSkip(TBuffer &b, const T &arrptr, const TCompInfo *compinfo,Int_t kase, TStreamerElement *aElement, Int_t narr, Int_t eoffset);
   template <class T>
   Int_t               ReadBufferConv(TBuffer &b, const T &arrptr, const TCompInfo *compinfo,Int_t kase, TStreamerElement *aElement, Int_t narr, Int_t eoffset);
   template <class T>
   Int_t               ReadBufferArtificial(TBuffer &b, const T &arrptr, TStreamerElement *aElement, Int_t narr, Int_t eoffset);

   Int_t               ReadBufferClones(TBuffer &b, TClonesArray *clones, Int_t nc, Int_t first, Int_t eoffset);
   Int_t               ReadBufferSTL(TBuffer &b, TVirtualCollectionProxy *cont, Int_t nc, Int_t eoffset, Bool_t v7 = kTRUE );
   void                SetCheckSum(UInt_t checksum) override { fCheckSum = checksum; }
   void                SetClass(TClass *cl) override;
   void                SetClassVersion(Int_t vers) override { fClassVersion = vers; }
   void                SetOnFileClassVersion(Int_t vers) { fOnFileClassVersion = vers; }
   void                TagFile(TFile *fFile) override;
private:
   // Try to remove those functions from the public interface.
   Int_t               WriteBuffer(TBuffer &b, char *pointer, Int_t first);
   Int_t               WriteBufferClones(TBuffer &b, TClonesArray *clones, Int_t nc, Int_t first, Int_t eoffset);
   Int_t               WriteBufferSTL   (TBuffer &b, TVirtualCollectionProxy *cont,   Int_t nc);
   Int_t               WriteBufferSTLPtrs( TBuffer &b, TVirtualCollectionProxy *cont, Int_t nc, Int_t first, Int_t eoffset);
public:
   void                Update(const TClass *oldClass, TClass *newClass) override;

   /// \brief Generate the TClass and TStreamerInfo for the requested pair.
   /// This creates a TVirtualStreamerInfo for the pair and trigger the BuildCheck/Old to
   /// provokes the creation of the corresponding TClass.  This relies on the dictionary for
   /// std::pair<const int, int> to already exist (or the interpreter information being available)
   /// as it is used as a template.
   /// \note The returned object is owned by the caller.
   TVirtualStreamerInfo *GenerateInfoForPair(const std::string &pairclassname, bool silent, size_t hint_pair_offset, size_t hint_pair_size) override;
   TVirtualStreamerInfo *GenerateInfoForPair(const std::string &firstname, const std::string &secondname, bool silent, size_t hint_pair_offset, size_t hint_pair_size) override;

   TVirtualCollectionProxy *GenEmulatedProxy(const char* class_name, Bool_t silent) override;
   TClassStreamer *GenEmulatedClassStreamer(const char* class_name, Bool_t silent) override;
   TVirtualCollectionProxy *GenExplicitProxy(const ::ROOT::Detail::TCollectionProxyInfo &info, TClass *cl) override;
   TClassStreamer *GenExplicitClassStreamer(const ::ROOT::Detail::TCollectionProxyInfo &info, TClass *cl) override;

   static TStreamerElement   *GetCurrentElement();

public:
   // For access by the StreamerInfoActions.
   template <class T>
   Int_t               WriteBufferAux      (TBuffer &b, const T &arr, TCompInfo *const*const compinfo, Int_t first, Int_t last, Int_t narr,Int_t eoffset,Int_t mode);

   //WARNING this class version must be the same as TVirtualStreamerInfo
   ClassDefOverride(TStreamerInfo, 10) // Streamer information for one class version
};


#endif
