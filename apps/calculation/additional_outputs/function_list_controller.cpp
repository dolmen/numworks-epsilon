#include "function_list_controller.h"

#include <apps/global_preferences.h>
#include <apps/shared/poincare_helpers.h>
#include <ion/unicode/code_point.h>
#include <poincare/code_point_layout.h>
#include <poincare/constant.h>
#include <poincare/expression.h>
#include <poincare/horizontal_layout.h>
#include <poincare/layout_helper.h>
#include <poincare/rational.h>
#include <poincare/symbol.h>
#include <poincare/vertical_offset_layout.h>
#include <string.h>

#include "../app.h"

using namespace Poincare;
using namespace Shared;

namespace Calculation {

void FunctionListController::setExactAndApproximateExpression(
    Poincare::Expression exactExpression,
    Poincare::Expression approximateExpression) {
  ExpressionsListController::setExpression(exactExpression);
  assert(!m_expression.isUninitialized());
  static_assert(
      k_maxNumberOfRows >= k_maxNumberOfOutputRows,
      "k_maxNumberOfRows must be greater than k_maxNumberOfOutputRows");

  Preferences* preferences = Preferences::sharedPreferences;
  Context* context = App::app()->localContext();

  float abscissa = exactExpression.getNumericalValue();
  Symbol variable = Symbol::SystemSymbol();
  exactExpression.replaceNumericalValuesWithSymbol(variable);

  Expression simplifiedExpression = exactExpression;
  PoincareHelpers::CloneAndSimplify(&simplifiedExpression, context,
                                    ReductionTarget::SystemForApproximation);

  /* Use the approximate expression to compute the ordinate to ensure that
   * it's coherent with the output of the calculation.
   * Sometimes when the reduction has some mistakes, the approximation of
   * simplifiedExpression(abscissa) can differ for the approximateExpression.
   */
  float ordinate = PoincareHelpers::ApproximateToScalar<float>(
      approximateExpression, context);
  m_model.setParameters(simplifiedExpression, abscissa, ordinate);

  m_layouts[0] = HorizontalLayout::Builder(
      LayoutHelper::String("y="),
      exactExpression
          .replaceSymbolWithExpression(variable, Symbol::Builder(k_symbol))
          .createLayout(preferences->displayMode(),
                        preferences->numberOfSignificantDigits(), context));
  setShowIllustration(true);
}

void FunctionListController::viewDidDisappear() {
  IllustratedExpressionsListController::viewDidDisappear();
  m_model.tidy();
}

I18n::Message FunctionListController::messageAtIndex(int index) {
  assert(index == 0);
  return I18n::Message::CurveEquation;
}

}  // namespace Calculation
