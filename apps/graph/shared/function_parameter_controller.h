#ifndef GRAPH_SHARED_FUNCTION_PARAMETER_CONTROLLER_H
#define GRAPH_SHARED_FUNCTION_PARAMETER_CONTROLLER_H

#include <apps/shared/column_parameter_controller.h>
#include <apps/shared/list_parameter_controller.h>
#include <escher/buffer_text_view.h>
#include <escher/chevron_view.h>
#include <escher/menu_cell.h>
#include <escher/message_text_view.h>
#include <escher/switch_view.h>

#include "../graph/graph_controller.h"
#include "details_parameter_controller.h"
#include "domain_parameter_controller.h"

namespace Graph {

class ValuesController;

class FunctionParameterController : public Shared::ListParameterController,
                                    public Shared::ColumnParameters {
 public:
  FunctionParameterController(Escher::Responder* parentResponder,
                              I18n::Message functionColorMessage,
                              I18n::Message deleteFunctionMessage);
  void setRecord(Ion::Storage::Record record) override;
  // MemoizedListViewDataSource
  Escher::HighlightCell* cell(int index) override;
  void fillCellForRow(Escher::HighlightCell* cell, int row) override;
  bool handleEvent(Ion::Events::Event event) override;
  int numberOfRows() const override { return k_numberOfRows; }
  const char* title() override;
  TitlesDisplay titlesDisplay() override {
    return TitlesDisplay::DisplayLastTwoTitles;
  }
  void setUseColumnTitle(bool useColumnTitle) {
    m_useColumnTitle = useColumnTitle;
  }

 private:
  // Shared cells + m_detailsCell + m_functionDomainCell + m_derivativeCell
  static constexpr int k_numberOfRows =
      Shared::ListParameterController::k_numberOfSharedCells + 3;
  bool displayDetails() const {
    assert(!Poincare::Preferences::sharedPreferences->examMode()
                .forbidGraphDetails() ||
           m_detailsParameterController.detailsNumberOfSections() == 0);
    return m_detailsParameterController.detailsNumberOfSections() > 0;
  }
  bool displayDomain() const {
    return m_domainParameterController.isVisible() > 0;
  }
  Shared::ExpiringPointer<Shared::ContinuousFunction> function();

  // ColumnParameters
  void initializeColumnParameters() override;
  Shared::ColumnNameHelper* columnNameHelper() override;

  Escher::MenuCell<Escher::MessageTextView, Escher::MessageTextView,
                   Escher::ChevronView>
      m_detailsCell;
  Escher::MenuCell<Escher::MessageTextView, Escher::OneLineBufferTextView<>,
                   Escher::ChevronView>
      m_functionDomainCell;
  Escher::MenuCell<Escher::MessageTextView, Escher::EmptyCellWidget,
                   Escher::SwitchView>
      m_derivativeCell;
  DetailsParameterController m_detailsParameterController;
  DomainParameterController m_domainParameterController;
  bool m_useColumnTitle;
};

}  // namespace Graph

#endif
