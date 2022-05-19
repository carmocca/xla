#include "torch_xla/csrc/ops/dynamic_ir.h"

#include "absl/strings/str_join.h"
#include "tensorflow/compiler/xla/xla_client/debug_macros.h"
#include "torch_xla/csrc/lowering_context.h"
#include "torch_xla/csrc/ops/infer_output_shape.h"

namespace torch_xla {

DimensionNode::DimensionNode(torch::lazy::OpKind op, OpList operands,
                             torch::lazy::hash_t hash_seed)
    : XlaNode(op, operands, xla::ShapeUtil::MakeShape(xla::S32, {}),
              /*num_outputs=*/1,
              /*hash_seed*/ hash_seed) {}

std::string DimensionNode::ToString() const { return "DimensionNode"; }

SizeNode::SizeNode(XlaValue input, size_t dim)
    : DimensionNode(
          torch::lazy::OpKind{c10::Symbol::fromQualString("aten::size")},
          {input}, torch::lazy::MHash(dim)),
      dim_(dim){};

XlaOpVector SizeNode::Lower(LoweringContext* loctx) const {
  auto input = loctx->GetOutputOp(operand(0));
  return ReturnOp(xla::GetDimensionSize(input, this->dim_), loctx);
}

int64_t SizeNode::getStaticValue() const {
  return dynamic_cast<const XlaNode*>(operand(0).node)->shape(0).size(dim_);
}

std::string SizeNode::ToString() const { return "SizeNode"; }

SizeAdd::SizeAdd(XlaValue a, XlaValue b)
    : DimensionNode(
          torch::lazy::OpKind{c10::Symbol::fromQualString("aten::add")},
          {a, b}){};

int64_t SizeAdd::getStaticValue() const {
  return dynamic_cast<const DimensionNode*>(operand(0).node)->getStaticValue() +
         dynamic_cast<const DimensionNode*>(operand(1).node)->getStaticValue();
}

std::string SizeAdd::ToString() const { return "SizeAdd"; }

XlaOpVector SizeAdd::Lower(LoweringContext* loctx) const {
  auto input1 = loctx->GetOutputOp(operand(0));
  auto input2 = loctx->GetOutputOp(operand(1));
  return ReturnOp(
      (xla::GetDimensionSize(input1, 0) + xla::GetDimensionSize(input2, 0)),
      loctx);
}

SizeMul::SizeMul(XlaValue a, XlaValue b)
    : DimensionNode(
          torch::lazy::OpKind{c10::Symbol::fromQualString("aten::mul")},
          {a, b}){};

int64_t SizeMul::getStaticValue() const {
  return dynamic_cast<const DimensionNode*>(operand(0).node)->getStaticValue() *
         dynamic_cast<const DimensionNode*>(operand(1).node)->getStaticValue();
}

std::string SizeMul::ToString() const { return "SizeMul"; }

XlaOpVector SizeMul::Lower(LoweringContext* loctx) const {
  auto input1 = loctx->GetOutputOp(operand(0));
  auto input2 = loctx->GetOutputOp(operand(1));
  return ReturnOp(xla::Mul(xla::GetDimensionSize(input1, 0),
                           xla::GetDimensionSize(input2, 0)),
                  loctx);
}

SizeDiv::SizeDiv(XlaValue a, XlaValue b)
    : DimensionNode(
          torch::lazy::OpKind{c10::Symbol::fromQualString("aten::div")},
          {a, b}){};

int64_t SizeDiv::getStaticValue() const {
  XLA_CHECK(
      dynamic_cast<const DimensionNode*>(operand(1).node)->getStaticValue() !=
      0)
      << "Can't divide a dimension by zero";
  return dynamic_cast<const DimensionNode*>(operand(0).node)->getStaticValue() /
         dynamic_cast<const DimensionNode*>(operand(1).node)->getStaticValue();
}

std::string SizeDiv::ToString() const { return "SizeDiv"; }

XlaOpVector SizeDiv::Lower(LoweringContext* loctx) const {
  auto input1 = loctx->GetOutputOp(operand(0));
  auto input2 = loctx->GetOutputOp(operand(1));
  return ReturnOp(xla::Div(xla::GetDimensionSize(input1, 0),
                           xla::GetDimensionSize(input2, 0)),
                  loctx);
}

}  // namespace torch_xla
