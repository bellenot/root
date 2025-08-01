#ifndef TMVA_SOFIE_ROPERATOR_GRU
#define TMVA_SOFIE_ROPERATOR_GRU

#include "TMVA/RModel.hxx"
#include "TMVA/ROperator.hxx"
#include "TMVA/SOFIE_common.hxx"

#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace TMVA {
namespace Experimental {
namespace SOFIE {

/*! \brief Gated Recurrent Unit operator
  *
  * Inference code generation for one-layer GRU. Supports forward, reverse and bidirectional GRU.
  * See the <a href="https://github.com/onnx/onnx/blob/master/docs/Operators.md#GRU">ONNX documentation</a>
  * for details about the supported GRU architectures.
  */
template <typename T> class ROperator_GRU final : public ROperator {
 private:
   std::vector<float> fAttrActivationAlpha;   ///< Scaling values used by some activation functions
   std::vector<float> fAttrActivationBeta;    ///< Scaling values used by some activation functions
   std::vector<std::string> fAttrActivations; ///< Activation functions
   float fAttrClip;                           ///< Clip threshold
   std::string fAttrDirection;                ///< Direction of processing
   size_t fAttrHiddenSize;                    ///< Number of the hidden layers
   size_t fAttrLayout;                        ///< Data layout
   size_t fAttrLinearBeforeReset;             ///< Linear layer before the reset gate

   std::string fNX;                           ///< Name of the input
   std::string fNW;                           ///< Name of the weights
   std::string fNR;                           ///< Name of the recurrence
   std::string fNB;                           ///< Name of the bias
   std::string fNSequence_lens;               ///< Name of the length of the sequences
   std::string fNInitial_h;                   ///< Name of the initial value of the hidden states
   std::string fNY;                           ///< Name of the output
   std::string fNY_h;                         ///< Name of the last sequence of the output

   std::vector<size_t> fShapeX;               ///< Shape of the input
   std::vector<size_t> fShapeW;               ///< Shape of the weights
   std::vector<size_t> fShapeR;               ///< Shape of the recurrence
   std::vector<size_t> fShapeB;               ///< Shape of the bias
   std::vector<size_t> fShapeSequence_lens;   ///< Shape of the length of the sequences
   std::vector<size_t> fShapeInitial_h;       ///< Shape of the initial value of hidden states
   std::vector<size_t> fShapeY;               ///< Shape of the output
   std::vector<size_t> fShapeY_h;             ///< Shape of the last sequence of the output

   std::string fType;                         ///< Type of the tensors

 public:
   /*! Default constructor of ROperator_GRU */
   ROperator_GRU() {}

   /*! \brief Constructor of ROperator_GRU from the attributes
    *
    * \param activation_alpha scaling values used by some activation functions
    * \param activation_beta scaling values used by some activation functions
    * \param activations activation functions
    * \param clip clip threshold
    * \param direction direction of processing of the sequneces
    * \param hidden_size number of hidden layers
    * \param layout data layout
    * \param linear_before_reset Linear layer before the reset gate
    * \param nameX name of the input tensor
    * \param nameW name of the weight tensor
    * \param nameR name of the recurrence tensor
    * \param nameB name of the bias tensor
    * \param nameSequence_lens name of the length of the sequences
    * \param nameInitial_h name of the initial value of the hidden states
    * \param nameY name of the output
    * \param nameY_h name of the last sequence of the output
    */
   ROperator_GRU(std::vector<float> activation_alpha,
                 std::vector<float> activation_beta,
                 std::vector<std::string> activations, float clip,
                 std::string direction, size_t hidden_size,
                 size_t layout, size_t linear_before_reset,
                 std::string nameX, std::string nameW, std::string nameR,
                 std::string nameB, std::string nameSequence_lens,
                 std::string nameInitial_h, std::string nameY, std::string nameY_h)
       : fAttrActivationAlpha(activation_alpha),
         fAttrActivationBeta(activation_beta), fAttrActivations(activations),
         fAttrClip(clip), fAttrDirection(direction), fAttrHiddenSize(hidden_size),
         fAttrLayout(layout), fAttrLinearBeforeReset(linear_before_reset),
         fNX(UTILITY::Clean_name(nameX)), fNW(UTILITY::Clean_name(nameW)),
         fNR(UTILITY::Clean_name(nameR)), fNB(UTILITY::Clean_name(nameB)),
         fNSequence_lens(UTILITY::Clean_name(nameSequence_lens)),
         fNInitial_h(UTILITY::Clean_name(nameInitial_h)),
         fNY(UTILITY::Clean_name(nameY)), fNY_h(UTILITY::Clean_name(nameY_h)) {

      fInputTensorNames = { fNX, fNW, fNR };
      if (!fNB.empty()){
        fInputTensorNames.emplace_back(fNB);
      }
      if (!fNSequence_lens.empty()){
        fInputTensorNames.emplace_back(fNSequence_lens);
      }
      if (!fNInitial_h.empty()){
        fInputTensorNames.emplace_back(fNInitial_h);
      }

      fOutputTensorNames = { };
      if (!fNY.empty()){
        fOutputTensorNames.emplace_back(fNY);
      }
      if (!fNY_h.empty()){
        fOutputTensorNames.emplace_back(fNY_h);
      }

      if (std::is_same<T, float>::value) {
         fType = "float";
      } else {
         throw std::runtime_error(
             "TMVA SOFIE Encountered unsupported type parsing a GRU operator");
      }
   }

   /*! \brief Infers the type of the output tensors
    *
    * \param input type of the input tensors
    */
   std::vector<ETensorType> TypeInference(std::vector<ETensorType> /*input*/) override;

   /*! \brief Infers the shape of the output tensors
    *
    * \param input shape of the input tensors
    */
   std::vector<std::vector<size_t>> ShapeInference(std::vector<std::vector<size_t>> /*input*/) override;

   /*! \brief Initialize the model
    *
    * \param model Model
    */
   void Initialize(RModel &) override;

   /*! \brief Generate the inference code
    *
    * \param OpName name of the operator
    */
   std::string Generate(std::string /*OpName*/) override;

   /*! \brief Returns the blas routines needed to compile the generated code
    */
   std::vector<std::string> GetBlasRoutines() override { return { std::string("Gemm"), std::string("Axpy") }; }
};

} // namespace SOFIE
} // namespace Experimental
} // namespace TMVA

// Implementation of the ROperator_GRU class
#include "TMVA/ROperator_GRU.icc"

#endif
