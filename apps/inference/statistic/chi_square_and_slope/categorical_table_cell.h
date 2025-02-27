#ifndef INFERENCE_STATISTIC_CHI_SQUARE_AND_SLOPE_CATEGORICAL_TABLE_CELL_H
#define INFERENCE_STATISTIC_CHI_SQUARE_AND_SLOPE_CATEGORICAL_TABLE_CELL_H

#include <escher/even_odd_editable_text_cell.h>
#include <escher/highlight_cell.h>
#include <escher/responder.h>
#include <escher/scroll_view.h>
#include <escher/selectable_table_view.h>
#include <shared/column_helper.h>
#include <shared/parameter_text_field_delegate.h>
#include <shared/text_field_delegate.h>

#include "inference/models/statistic/statistic.h"
#include "inference/models/statistic/table.h"
#include "inference/shared/dynamic_cells_data_source.h"
#include "inference/statistic/chi_square_and_slope/categorical_table_view_data_source.h"

namespace Inference {

class CategoricalController;

class CategoricalTableCell : public Escher::HighlightCell,
                             public Escher::Responder,
                             public Escher::SelectableTableViewDataSource,
                             public Escher::SelectableTableViewDelegate {
 public:
  CategoricalTableCell(Escher::Responder *parentResponder,
                       Escher::TableViewDataSource *dataSource);

  // Responder
  void didBecomeFirstResponder() override;
  bool handleEvent(Ion::Events::Event e) override;

  // HighlightCell
  void reloadCell() override {}
  Responder *responder() override { return this; }

  Escher::SelectableTableView *selectableTableView() {
    return &m_selectableTableView;
  }
  virtual CategoricalTableViewDataSource *tableViewDataSource() = 0;

  KDSize minimalSizeForOptimalDisplay() const override {
    return m_selectableTableView.minimalSizeForOptimalDisplay();
  }

  // SelectableTableViewDelegate
  void tableViewDidChangeSelectionAndDidScroll(
      Escher::SelectableTableView *t, int previousSelectedCol,
      int previousSelectedRow, KDPoint previousOffset,
      bool withinTemporarySelection = false) override;
  bool canStoreContentOfCellAtLocation(Escher::SelectableTableView *t, int col,
                                       int row) const override {
    return row > 0;
  }

  void layoutSubviews(bool force = false) override;

 protected:
  constexpr static int k_bottomMargin = 5;
  virtual CategoricalController *categoricalController() = 0;

  Escher::SelectableTableView m_selectableTableView;

 private:
  int numberOfSubviews() const override { return 1; }
  Escher::View *subviewAtIndex(int i) override {
    return &m_selectableTableView;
  }
};

class InputCategoricalTableCell
    : public CategoricalTableCell,
      public Shared::TextFieldDelegate,
      public DynamicCellsDataSourceDelegate<InferenceEvenOddEditableCell>,
      public Shared::ClearColumnHelper {
 public:
  InputCategoricalTableCell(Escher::Responder *parentResponder,
                            Escher::TableViewDataSource *dataSource,
                            Statistic *statistic);

  // TextFieldDelegate
  bool textFieldShouldFinishEditing(Escher::AbstractTextField *textField,
                                    Ion::Events::Event event) override;
  bool textFieldDidFinishEditing(Escher::AbstractTextField *textField,
                                 const char *text,
                                 Ion::Events::Event event) override;

  // Responder
  bool handleEvent(Ion::Events::Event e) override;

  // DynamicCellsDataSourceDelegate<InferenceEvenOddEditableCell>
  void initCell(InferenceEvenOddEditableCell, void *cell, int index) override;

  virtual bool recomputeDimensions();

 protected:
  // ClearColumnHelper
  Escher::SelectableTableView *table() override {
    return selectableTableView();
  }
  int numberOfElementsInColumn(int column) const override;
  void clearSelectedColumn() override;
  bool isColumnClearable(int column) override {
    return relativeColumn(column) >= 0;
  }

  virtual int relativeColumn(int column) const = 0;
  int relativeRow(int row) { return row - 1; }
  bool deleteSelectedValue();
  Table *tableModel();
  const Table *constTableModel() const {
    return const_cast<InputCategoricalTableCell *>(this)->tableModel();
  }

  Statistic *m_statistic;
  int m_numberOfRows;
  int m_numberOfColumns;
};

class DoubleColumnTableCell
    : public InputCategoricalTableCell,
      public CategoricalTableViewDataSource,
      public DynamicCellsDataSource<InferenceEvenOddEditableCell,
                                    k_doubleColumnTableNumberOfReusableCells> {
 public:
  DoubleColumnTableCell(Escher::Responder *parentResponder,
                        Statistic *statistic);

  // InputCategoricalTableCell
  CategoricalTableViewDataSource *tableViewDataSource() override {
    return this;
  }

  // DataSource
  int numberOfRows() const override {
    return m_numberOfRows + 1;
  }  // Add header
  int numberOfColumns() const override { return m_numberOfColumns; }
  int reusableCellCount(int type) override;
  Escher::HighlightCell *reusableCell(int i, int type) override;
  int typeAtLocation(int column, int row) override {
    return row == 0 ? k_typeOfHeaderCells : k_typeOfInnerCells;
  }
  void fillCellForLocation(Escher::HighlightCell *cell, int column,
                           int row) override;

  // DynamicCellsDataSource
  Escher::SelectableTableView *tableView() override {
    return &m_selectableTableView;
  }

  constexpr static int k_maxNumberOfColumns = 2;
  constexpr static int k_numberOfReusableCells =
      k_maxNumberOfColumns * k_maxNumberOfReusableRows;

 private:
  KDCoordinate nonMemoizedColumnWidth(int column) override {
    return k_columnWidth;
  }
  // CategoricalTableViewDataSource
  int relativeColumn(int column) const override { return column; }
  virtual Escher::HighlightCell *headerCell(int index) = 0;
};

}  // namespace Inference

#endif
