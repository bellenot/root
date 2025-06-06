#include "ntuple_test.hxx"

#include <TMath.h>
#include <TObject.h>
#include <TRef.h>
#include <TRotation.h>

#include <memory>
#include <sstream>

namespace {
class RNoDictionary {};
} // namespace

namespace ROOT {
template <>
struct IsCollectionProxy<CyclicCollectionProxy> : std::true_type {
};
} // namespace ROOT

TEST(RNTuple, TClass) {
   auto modelFail = RNTupleModel::Create();
   EXPECT_THROW(modelFail->MakeField<RNoDictionary>("nodict"), ROOT::RException);

   auto model = RNTupleModel::Create();
   auto ptrKlass = model->MakeField<CustomStruct>("klass");

   // TDatime would be a supported class layout but it is blocked due to its custom streamer
   EXPECT_THROW(model->MakeField<TDatime>("datime"), ROOT::RException);

   FileRaii fileGuard("test_ntuple_tclass.root");
   auto ntuple = RNTupleWriter::Recreate(std::move(model), "f", fileGuard.GetPath());
}

TEST(RNTuple, CyclicClass)
{
   auto modelFail = RNTupleModel::Create();
   EXPECT_THROW(modelFail->MakeField<Cyclic>("cyclic"), ROOT::RException);

   CyclicCollectionProxy ccp;
   auto cl = TClass::GetClass("CyclicCollectionProxy");
   cl->CopyCollectionProxy(ccp);
   EXPECT_THROW(RFieldBase::Create("f", "CyclicCollectionProxy").Unwrap(), ROOT::RException);
}

TEST(RNTuple, DiamondInheritance)
{
   FileRaii fileGuard("test_ntuple_diamond_inheritance.root");

   {
      auto model = RNTupleModel::Create();
      auto d = model->MakeField<DuplicateBaseD>("d");
      EXPECT_THROW(model->MakeField<DiamondVirtualD>("vd"), ROOT::RException);
      auto writer = RNTupleWriter::Recreate(std::move(model), "ntpl", fileGuard.GetPath());
      d->DuplicateBaseB::a = 1.0;
      d->DuplicateBaseC::a = 1.5;
      d->b = 2.0;
      d->c = 3.0;
      d->d = 4.0;
      writer->Fill();
   }

   auto reader = RNTupleReader::Open("ntpl", fileGuard.GetPath());
   auto d = reader->GetModel().GetDefaultEntry().GetPtr<DuplicateBaseD>("d");
   EXPECT_EQ(1u, reader->GetNEntries());

   reader->LoadEntry(0);
   EXPECT_FLOAT_EQ(1.0, d->DuplicateBaseB::a);
   EXPECT_FLOAT_EQ(1.5, d->DuplicateBaseC::a);
   EXPECT_FLOAT_EQ(2.0, d->b);
   EXPECT_FLOAT_EQ(3.0, d->c);
   EXPECT_FLOAT_EQ(4.0, d->d);
}

TEST(RNTuple, TObject)
{
   // Ensure that TObject cannot be accidentally handled through the generic RClassField field
   EXPECT_THROW(ROOT::RClassField("obj", "TObject"), ROOT::RException);

   FileRaii fileGuard("test_ntuple_tobject.root");
   {
      auto model = RNTupleModel::Create();
      model->MakeField<TObject>("obj");
      auto writer = RNTupleWriter::Recreate(std::move(model), "ntpl", fileGuard.GetPath());
      auto entry = writer->GetModel().CreateBareEntry();

      auto heapObj = std::make_unique<TObject>();
      EXPECT_TRUE(heapObj->TestBit(TObject::kIsOnHeap));
      EXPECT_TRUE(heapObj->TestBit(TObject::kNotDeleted));
      heapObj->SetUniqueID(137);
      entry->BindRawPtr("obj", heapObj.get());
      writer->Fill(*entry);

      // Saving a destructed object is here to verify that the RNTuple serialization does the same than
      // the TObject custom streamer (i.e., ignoring the kNotDeleted flag)
      heapObj->~TObject();
      EXPECT_FALSE(heapObj->TestBit(TObject::kNotDeleted));
      writer->Fill(*entry);

      TObject stackObj;
      EXPECT_FALSE(stackObj.TestBit(TObject::kIsOnHeap));
      entry->BindRawPtr("obj", &stackObj);
      writer->Fill(*entry);
   }

   auto reader = RNTupleReader::Open("ntpl", fileGuard.GetPath());
   EXPECT_EQ(3u, reader->GetNEntries());

   auto entry = reader->GetModel().CreateBareEntry();
   TObject stackObj;
   entry->BindRawPtr("obj", &stackObj);

   reader->LoadEntry(0, *entry);
   EXPECT_EQ(137u, stackObj.GetUniqueID());
   EXPECT_FALSE(stackObj.TestBit(TObject::kIsOnHeap));
   EXPECT_TRUE(stackObj.TestBit(TObject::kNotDeleted));

   reader->LoadEntry(1, *entry);
   EXPECT_EQ(137u, stackObj.GetUniqueID());
   EXPECT_FALSE(stackObj.TestBit(TObject::kIsOnHeap));
   EXPECT_TRUE(stackObj.TestBit(TObject::kNotDeleted));

   auto heapObj = std::make_unique<TObject>();
   entry->BindRawPtr("obj", heapObj.get());
   reader->LoadEntry(2, *entry);
   EXPECT_EQ(0u, heapObj->GetUniqueID());
   EXPECT_TRUE(heapObj->TestBit(TObject::kIsOnHeap));
   EXPECT_TRUE(heapObj->TestBit(TObject::kNotDeleted));
}

TEST(RNTuple, TObjectReferenced)
{
   // RNTuple does not support reading nor writing of TObject objects that are marked as referenced
   FileRaii fileGuard("test_ntuple_tobject_referenced.root");
   {
      auto model = RNTupleModel::Create();
      auto ptrObject = model->MakeField<TObject>("obj");
      auto writer = RNTupleWriter::Recreate(std::move(model), "ntpl", fileGuard.GetPath());
      writer->Fill();

      ptrObject->SetBit(TObject::kIsReferenced);
      EXPECT_THROW(writer->Fill(), ROOT::RException);
   }

   auto reader = RNTupleReader::Open("ntpl", fileGuard.GetPath());
   EXPECT_EQ(1u, reader->GetNEntries());
   auto ptrObject = reader->GetModel().GetDefaultEntry().GetPtr<TObject>("obj");

   reader->LoadEntry(0);
   EXPECT_EQ(0u, ptrObject->GetUniqueID());
   ptrObject->SetBit(TObject::kIsReferenced);
   EXPECT_THROW(reader->LoadEntry(0), ROOT::RException);
}

TEST(RNTuple, TObjectShow)
{
   FileRaii fileGuard("test_ntuple_tobject_show.root");
   {
      auto model = RNTupleModel::Create();
      auto ptrObject = model->MakeField<TObject>("obj");
      auto writer = RNTupleWriter::Recreate(std::move(model), "ntpl", fileGuard.GetPath());
      ptrObject->SetUniqueID(137);
      writer->Fill();
   }

   auto reader = RNTupleReader::Open("ntpl", fileGuard.GetPath());
   // clang-format off
   std::string expected{
      "{\n"
      "  \"obj\": {\n"
      "    \"fUniqueID\": 137,\n"
      "    \"fBits\": 33554432\n"
      "  }\n"
      "}\n"
   };
   // clang-format on
   std::ostringstream os;
   reader->Show(0, os);
   EXPECT_EQ(expected, os.str());
}

TEST(RNTuple, TObjectDerived)
{
   FileRaii fileGuard("test_ntuple_tobject_derived.root");

   {
      auto model = RNTupleModel::Create();
      // The choice of TRotation is arbitrary; it is a simple, existing class that inherits from TObject
      // and is supported by RNTuple
      auto ptrRotation = model->MakeField<TRotation>("rotation");
      ptrRotation->RotateX(TMath::Pi());
      ptrRotation->SetUniqueID(137);
      auto ptrMultiple = model->MakeField<DerivedFromLeftAndTObject>("derived");
      ptrMultiple->SetUniqueID(137);
      auto writer = RNTupleWriter::Recreate(std::move(model), "ntpl", fileGuard.GetPath());
      writer->Fill();
   }

   auto reader = RNTupleReader::Open("ntpl", fileGuard.GetPath());
   EXPECT_EQ(1u, reader->GetNEntries());

   auto ptrRotation = reader->GetModel().GetDefaultEntry().GetPtr<TRotation>("rotation");
   auto ptrMultiple = reader->GetModel().GetDefaultEntry().GetPtr<DerivedFromLeftAndTObject>("derived");
   reader->LoadEntry(0);

   EXPECT_DOUBLE_EQ(1.0, ptrRotation->XX());
   EXPECT_EQ(137u, ptrRotation->GetUniqueID());

   EXPECT_FLOAT_EQ(1.0, ptrMultiple->x);
   EXPECT_EQ(137u, ptrMultiple->GetUniqueID());
}

TEST(RNTuple, TClassTypeChecksum)
{
   auto f0 = RFieldBase::Create("f0", "std::vector<int>").Unwrap();
   EXPECT_FALSE(f0->GetTraits() & RFieldBase::kTraitTypeChecksum);
   EXPECT_EQ(0u, f0->GetTypeChecksum());

   auto f1 = RFieldBase::Create("f1", "CustomStruct").Unwrap();
   EXPECT_TRUE(f1->GetTraits() & RFieldBase::kTraitTypeChecksum);
   EXPECT_EQ(TClass::GetClass("CustomStruct")->GetCheckSum(), f1->GetTypeChecksum());

   auto f2 = std::make_unique<ROOT::RStreamerField>("f2", "TRotation");
   EXPECT_TRUE(f2->GetTraits() & RFieldBase::kTraitTypeChecksum);
   EXPECT_EQ(TClass::GetClass("TRotation")->GetCheckSum(), f2->GetTypeChecksum());

   auto f3 = RFieldBase::Create("f1", "TObject").Unwrap();
   EXPECT_TRUE(f3->GetTraits() & RFieldBase::kTraitTypeChecksum);
   EXPECT_EQ(TClass::GetClass("TObject")->GetCheckSum(), f3->GetTypeChecksum());
}

TEST(RNTuple, TClassReadRules)
{
   ROOT::TestSupport::CheckDiagsRAII diags;
   diags.requiredDiag(kWarning, "[ROOT.NTuple]",
                      "ignoring I/O customization rule due to conflicting source member type: float vs. double "
                      "for member a",
                      false);

   // Zero out least significant 8 bits
   float last8BitsZero = 2.0;
   std::uint32_t bits;
   std::memcpy(&bits, &last8BitsZero, sizeof(bits));
   bits &= ~0xFF;
   std::memcpy(&last8BitsZero, &bits, sizeof(last8BitsZero));

   FileRaii fileGuard("test_ntuple_tclassrules.root");
   char c[4] = {'R', 'O', 'O', 'T'};
   {
      auto model = RNTupleModel::Create();
      auto ptrClass = model->MakeField<StructWithIORules>("class");
      auto ptrCoord = model->MakeField<CoordinatesWithIORules>("coord");
      auto ptrOldCoord = model->MakeField<OldCoordinates>("oldCoord");
      auto ptrLowPrecisionFloat = model->MakeField<LowPrecisionFloatWithIORules>("lowPrecisionFloat");
      auto ptrOldName = model->MakeField<OldName<OldName<int>>>("rename");
      auto ptrWithSource = model->MakeField<StructWithSourceStruct>("withSource");
      ptrCoord->fX = ptrOldCoord->fOldX = 1.0;
      ptrCoord->fY = ptrOldCoord->fOldY = 1.0;
      ptrLowPrecisionFloat->fFoo = 1.0;
      ptrLowPrecisionFloat->fLast8BitsZero = last8BitsZero;
      ptrOldName->fValue.fValue = 42;
      // The following two members are transient and should not be stored.
      ptrWithSource->fSource.fTransient = 1;
      ptrWithSource->fTransient = 2;
      auto writer = RNTupleWriter::Recreate(std::move(model), "f", fileGuard.GetPath());
      for (int i = 0; i < 5; i++) {
         *ptrClass = StructWithIORules{/*a=*/static_cast<float>(i), /*chars=*/c};
         ptrWithSource->fSource.fValue = i;
         writer->Fill();
      }
   }

   auto reader = RNTupleReader::Open("f", fileGuard.GetPath());
   EXPECT_EQ(5U, reader->GetNEntries());
   EXPECT_EQ(TClass::GetClass("StructWithIORules")->GetCheckSum(),
             reader->GetModel().GetConstField("class").GetOnDiskTypeChecksum());
   auto viewKlass = reader->GetView<StructWithIORules>("class");
   auto viewWithSource = reader->GetView<StructWithSourceStruct>("withSource");
   for (auto i : reader->GetEntryRange()) {
      float fi = static_cast<float>(i);
      EXPECT_EQ(fi, viewKlass(i).a);
      EXPECT_TRUE(0 == memcmp(c, viewKlass(i).s.chars, sizeof(c)));

      // The following values are set from a read rule; see CustomStructLinkDef.h
      EXPECT_FLOAT_EQ(fi + 1.0f, viewKlass(i).b);
      EXPECT_FLOAT_EQ(viewKlass(i).a + viewKlass(i).b, viewKlass(i).c);
      EXPECT_FLOAT_EQ(2 * (viewKlass(i).a + viewKlass(i).b), viewKlass(i).cDerived);
      EXPECT_STREQ("ROOT", viewKlass(i).s.str.c_str());

      // The following member is set by a checksum based rule
      EXPECT_FLOAT_EQ(42.0, viewKlass(i).checksumA);
      // The following member is not touched by a rule due to a checksum mismatch
      EXPECT_FLOAT_EQ(137.0, viewKlass(i).checksumB);

      // The staging area should have called the constructor and set fSource.fTransient = 23; fSource.fValue = i is
      // loaded from disk, and then fTransient should be 23 + i
      EXPECT_EQ(23 + i, viewWithSource(i).fTransient);
   }

   auto viewCoord = reader->GetView<CoordinatesWithIORules>("coord");
   EXPECT_FLOAT_EQ(1.0, viewCoord(0).fX);
   EXPECT_FLOAT_EQ(1.0, viewCoord(0).fY);
   EXPECT_FLOAT_EQ(sqrt(2), viewCoord(0).fR);
   EXPECT_FLOAT_EQ(M_PI / 4., viewCoord(0).fPhi);

   auto viewLowPrecisionFloat = reader->GetView<LowPrecisionFloatWithIORules>("lowPrecisionFloat");
   EXPECT_FLOAT_EQ(1.0, viewLowPrecisionFloat(0).fFoo);
   EXPECT_NE(last8BitsZero, viewLowPrecisionFloat(0).fLast8BitsZero);
   EXPECT_NEAR(2.0, viewLowPrecisionFloat(0).fLast8BitsZero, 0.001);
   auto viewOldCoordTransformed = reader->GetView<CoordinatesWithIORules>("oldCoord");
   EXPECT_FLOAT_EQ(1.0, viewOldCoordTransformed(0).fX);
   EXPECT_FLOAT_EQ(1.0, viewOldCoordTransformed(0).fY);
   EXPECT_FLOAT_EQ(sqrt(2), viewOldCoordTransformed(0).fR);
   EXPECT_FLOAT_EQ(M_PI / 4., viewOldCoordTransformed(0).fPhi);

   auto viewRename = reader->GetView<NewName<OldName<int>>>("rename");
   EXPECT_EQ(42, viewRename(0).fValue.fValue);
}
