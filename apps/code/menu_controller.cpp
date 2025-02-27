#include "menu_controller.h"

#include <apps/i18n.h>
#include <apps/shared/expression_model_list_controller.h>
#include <assert.h>
#include <escher/metric.h>
#include <ion/events.h>
#include <ion/unicode/utf8_decoder.h>

#include "../apps_container.h"
#include "app.h"

using namespace Escher;

namespace Code {

MenuController::MenuController(Responder *parentResponder, App *pythonDelegate,
                               ScriptStore *scriptStore,
                               ButtonRowController *footer)
    : ViewController(parentResponder),
      RegularHeightTableViewDataSource(),
      ButtonRowDelegate(nullptr, footer),
      m_scriptStore(scriptStore),
      m_addNewScriptCell(KDGlyph::Format{}),
      m_consoleButton(
          this, I18n::Message::Console,
          Invocation::Builder<MenuController>(
              [](MenuController *menu, void *sender) {
                menu->consoleController()->setAutoImport(true);
                menu->stackViewController()->push(menu->consoleController());
                return true;
              },
              this),
          KDFont::Size::Large),
      m_selectableTableView(this, this, this, this),
      m_scriptParameterController(nullptr, I18n::Message::ScriptOptions, this),
      m_editorController(this, pythonDelegate),
      m_reloadConsoleWhenBecomingFirstResponder(false),
      m_shouldDisplayAddScriptRow(true) {
  m_selectableTableView.setMargins(0);
  m_selectableTableView.hideScrollBars();
  m_addNewScriptCell.setLeftMargin(
      Shared::ExpressionModelListController::k_newModelMargin);
  m_addNewScriptCell.setMessage(I18n::Message::AddScript);
  for (int i = 0; i < k_maxNumberOfDisplayableScriptCells; i++) {
    m_scriptCells[i].setParentResponder(&m_selectableTableView);
    m_scriptCells[i].textField()->setDelegates(nullptr, this);
  }
}

ConsoleController *MenuController::consoleController() {
  return App::app()->consoleController();
}

StackViewController *MenuController::stackViewController() {
  return static_cast<StackViewController *>(
      parentResponder()->parentResponder());
}

void MenuController::willExitResponderChain(Responder *nextFirstResponder) {
  forceTextFieldEditionToAbort(false);
}

void MenuController::didBecomeFirstResponder() {
  if (m_reloadConsoleWhenBecomingFirstResponder) {
    reloadConsole();
  }
  if (footer()->selectedButton() == 0) {
    assert(m_selectableTableView.selectedRow() < 0);
    Container::activeApp()->setFirstResponder(&m_consoleButton);
    return;
  }
  if (m_selectableTableView.selectedRow() < 0) {
    m_selectableTableView.selectCellAtLocation(0, 0);
  }
  assert(m_selectableTableView.selectedRow() <
         m_scriptStore->numberOfScripts() + 1);
  Container::activeApp()->setFirstResponder(&m_selectableTableView);
#if EPSILON_GETOPT
  if (consoleController()->locked()) {
    consoleController()->setAutoImport(true);
    stackViewController()->push(consoleController());
    return;
  }
#endif
}

void MenuController::viewWillAppear() {
  ViewController::viewWillAppear();
  updateAddScriptRowDisplay();
}

bool MenuController::handleEvent(Ion::Events::Event event) {
  if (event == Ion::Events::Down) {
    m_selectableTableView.deselectTable();
    footer()->setSelectedButton(0);
    return true;
  }
  if (event == Ion::Events::Up) {
    if (footer()->selectedButton() == 0) {
      footer()->setSelectedButton(-1);
      m_selectableTableView.selectCellAtLocation(0, numberOfRows() - 1);
      Container::activeApp()->setFirstResponder(&m_selectableTableView);
      return true;
    }
  }
  if (event == Ion::Events::OK || event == Ion::Events::EXE) {
    int selectedRow = m_selectableTableView.selectedRow();
    int selectedColumn = m_selectableTableView.selectedColumn();
    if (selectedRow >= 0 && selectedRow < m_scriptStore->numberOfScripts()) {
      if (selectedColumn == 1) {
        configureScript();
        return true;
      }
      assert(selectedColumn == 0);
      editScriptAtIndex(selectedRow);
      return true;
    } else if (m_shouldDisplayAddScriptRow && selectedColumn == 0 &&
               selectedRow == m_scriptStore->numberOfScripts()) {
      addScript();
      return true;
    }
  }
  return false;
}

void MenuController::renameSelectedScript() {
  assert(m_selectableTableView.selectedRow() >= 0);
  assert(m_selectableTableView.selectedRow() <
         m_scriptStore->numberOfScripts());
  AppsContainer::sharedAppsContainer()->setShiftAlphaStatus(
      Ion::Events::ShiftAlphaStatus::AlphaLock);
  m_selectableTableView.selectCellAtLocation(
      0, (m_selectableTableView.selectedRow()));
  ScriptNameCell *myCell =
      static_cast<ScriptNameCell *>(m_selectableTableView.selectedCell());
  Container::activeApp()->setFirstResponder(myCell);
  myCell->setHighlighted(false);
  TextField *tf = myCell->textField();
  const char *previousText = tf->text();
  tf->setEditing(true);
  tf->setText(previousText);
  tf->setCursorLocation(tf->text() + strlen(previousText));
}

void MenuController::deleteScript(Script script) {
  assert(!script.isNull());
  script.destroy();
  updateAddScriptRowDisplay();
}

void MenuController::reloadConsole() {
  consoleController()->unloadPythonEnvironment();
  m_reloadConsoleWhenBecomingFirstResponder = false;
}

void MenuController::openConsoleWithScript(Script script) {
  reloadConsole();
  consoleController()->setAutoImport(false);
  stackViewController()->push(consoleController());
  consoleController()->autoImportScript(script, true);
  m_reloadConsoleWhenBecomingFirstResponder = true;
}

void MenuController::scriptContentEditionDidFinish() { reloadConsole(); }

void MenuController::willExitApp() {
  forceTextFieldEditionToAbort(false);
  m_editorController.willExitApp();
}

int MenuController::numberOfRows() const {
  return m_scriptStore->numberOfScripts() + m_shouldDisplayAddScriptRow;
}

void MenuController::fillCellForLocation(HighlightCell *cell, int column,
                                         int row) {
  if (column == 0 && row < m_scriptStore->numberOfScripts()) {
    willDisplayScriptTitleCellForIndex(cell, row);
  }
  static_cast<EvenOddCell *>(cell)->setEven(row % 2 == 0);
}

KDCoordinate MenuController::nonMemoizedColumnWidth(int column) {
  if (column == 0) {
    return m_selectableTableView.bounds().width() - k_parametersColumnWidth;
  }
  assert(column == 1);
  return k_parametersColumnWidth;
}

HighlightCell *MenuController::reusableCell(int index, int type) {
  assert(index >= 0);
  if (type == k_scriptCellType) {
    assert(index >= 0 && index < k_maxNumberOfDisplayableScriptCells);
    return &m_scriptCells[index];
  }
  if (type == k_scriptParameterCellType) {
    assert(index >= 0 && index < k_maxNumberOfDisplayableScriptCells);
    return &m_scriptParameterCells[index];
  }
  if (type == k_addScriptCellType) {
    assert(index == 0);
    return &m_addNewScriptCell;
  }
  assert(type == k_emptyCellType);
  return &m_emptyCell;
}

int MenuController::reusableCellCount(int type) {
  if (type == k_addScriptCellType) {
    return 1;
  }
  if (type == k_scriptCellType || type == k_scriptParameterCellType) {
    return k_maxNumberOfDisplayableScriptCells;
  }
  assert(type == k_emptyCellType);
  return 1;
}

int MenuController::typeAtLocation(int column, int row) {
  assert(column >= 0 && column < numberOfColumns());
  assert(row >= 0 && row < numberOfRows());
  if (column == 0) {
    if (row == numberOfRows() - 1 && m_shouldDisplayAddScriptRow) {
      return k_addScriptCellType;
    }
    return k_scriptCellType;
  }
  assert(column == 1);
  if (row == numberOfRows() - 1 && m_shouldDisplayAddScriptRow) {
    return k_emptyCellType;
  }
  return k_scriptParameterCellType;
}

void MenuController::willDisplayScriptTitleCellForIndex(HighlightCell *cell,
                                                        int index) {
  assert(index >= 0 && index < m_scriptStore->numberOfScripts());
  (static_cast<ScriptNameCell *>(cell))
      ->textField()
      ->setText(m_scriptStore->scriptAtIndex(index).fullName());
}

void MenuController::tableViewDidChangeSelectionAndDidScroll(
    SelectableTableView *t, int previousSelectedCol, int previousSelectedRow,
    KDPoint previousOffset, bool withinTemporarySelection) {
  assert(t == &m_selectableTableView);
  if (selectedRow() == numberOfRows() - 1 && selectedColumn() == 1 &&
      m_shouldDisplayAddScriptRow) {
    t->selectCellAtLocation(0, numberOfRows() - 1);
  } else {
    // Copy the selection state of addModelCell to make it look as a single cell
    m_emptyCell.setHighlighted(selectedRow() == numberOfRows() - 1);
  }
}

bool MenuController::textFieldShouldFinishEditing(AbstractTextField *textField,
                                                  Ion::Events::Event event) {
  return event == Ion::Events::OK || event == Ion::Events::EXE ||
         event == Ion::Events::Down || event == Ion::Events::Up;
}

bool MenuController::textFieldDidFinishEditing(AbstractTextField *textField,
                                               const char *text,
                                               Ion::Events::Event event) {
  const char *newName;
  //"script99" + "." + "py"
  constexpr static int bufferSize = Script::k_defaultScriptNameMaxSize + 1 +
                                    ScriptStore::k_scriptExtensionLength;
  char numberedDefaultName[bufferSize];

  if (strlen(text) > 1 + strlen(ScriptStore::k_scriptExtension)) {
    newName = text;
  } else {
    // The user entered an empty name. Use a numbered default script name.
    bool foundDefaultName = Script::DefaultName(
        numberedDefaultName, Script::k_defaultScriptNameMaxSize);
    int defaultNameLength = strlen(numberedDefaultName);
    assert(UTF8Decoder::CharSizeOfCodePoint('.') == 1);
    numberedDefaultName[defaultNameLength++] = '.';
    assert(defaultNameLength < bufferSize);
    strlcpy(numberedDefaultName + defaultNameLength,
            ScriptStore::k_scriptExtension, bufferSize - defaultNameLength);
    /* If there are already scripts named script1.py, script2.py,... until
     * Script::k_maxNumberOfDefaultScriptNames, we want to write the last tried
     * default name and let the user modify it. */
    if (!foundDefaultName) {
      textField->setText(numberedDefaultName);
      textField->setCursorLocation(textField->draftTextBuffer() +
                                   defaultNameLength);
    }
    newName = const_cast<const char *>(numberedDefaultName);
  }
  Script script =
      m_scriptStore->scriptAtIndex(m_selectableTableView.selectedRow());
  Script::ErrorStatus error =
      Script::NameCompliant(newName)
          ? Ion::Storage::Record::SetFullName(&script, newName)
          : Script::ErrorStatus::NonCompliantName;
  if (error == Script::ErrorStatus::None) {
    updateAddScriptRowDisplay();
    textField->setText(newName);
    int currentRow = m_selectableTableView.selectedRow();
    if (event == Ion::Events::Down && currentRow < numberOfRows() - 1) {
      m_selectableTableView.selectCellAtLocation(
          m_selectableTableView.selectedColumn(), currentRow + 1);
    } else if (event == Ion::Events::Up && currentRow > 0) {
      m_selectableTableView.selectCellAtLocation(
          m_selectableTableView.selectedColumn(), currentRow - 1);
    }
    reloadConsole();
    Container::activeApp()->setFirstResponder(&m_selectableTableView);
    AppsContainer::sharedAppsContainer()->setShiftAlphaStatus(
        Ion::Events::ShiftAlphaStatus::Default);
    return true;
  } else if (error == Script::ErrorStatus::NameTaken) {
    Container::activeApp()->displayWarning(I18n::Message::NameTaken);
  } else if (error == Script::ErrorStatus::NonCompliantName) {
    Container::activeApp()->displayWarning(
        I18n::Message::AllowedCharactersaz09,
        I18n::Message::NameCannotStartWithNumber);
  } else {
    assert(error == Script::ErrorStatus::NotEnoughSpaceAvailable);
    Container::activeApp()->displayWarning(I18n::Message::NameTooLong);
  }
  return false;
}

bool MenuController::textFieldDidHandleEvent(AbstractTextField *textField,
                                             bool returnValue,
                                             bool textDidChange) {
  int scriptExtensionLength = 1 + strlen(ScriptStore::k_scriptExtension);
  if (textField->isEditing()) {
    const char *maxPointerLocation = textField->text() +
                                     textField->draftTextLength() -
                                     scriptExtensionLength;
    if (textField->cursorLocation() > maxPointerLocation) {
      textField->setCursorLocation(maxPointerLocation);
    }
  }
  return returnValue;
}

void MenuController::addScript() {
  Script::ErrorStatus error = m_scriptStore->addNewScript();
  /* Adding a new script is called when !m_scriptStore.isFull() which guarantees
   * that the available space in the storage is big enough */
  assert(error == Script::ErrorStatus::None);
  (void)error;  // Silence the compiler
  updateAddScriptRowDisplay();
  renameSelectedScript();
}

void MenuController::configureScript() {
  assert(m_selectableTableView.selectedRow() >= 0);
  assert(m_selectableTableView.selectedRow() <
         m_scriptStore->numberOfScripts());
  m_scriptParameterController.setScript(
      m_scriptStore->scriptAtIndex(m_selectableTableView.selectedRow()));
  stackViewController()->push(&m_scriptParameterController);
}

void MenuController::editScriptAtIndex(int scriptIndex) {
  assert(scriptIndex >= 0 && scriptIndex < m_scriptStore->numberOfScripts());
  Script script = m_scriptStore->scriptAtIndex(scriptIndex);
  m_editorController.setScript(script, scriptIndex);
  stackViewController()->push(&m_editorController);
}

void MenuController::updateAddScriptRowDisplay() {
  m_shouldDisplayAddScriptRow = !m_scriptStore->isFull();
  m_selectableTableView.reloadData();
}

bool MenuController::privateTextFieldDidAbortEditing(
    AbstractTextField *textField, bool menuControllerStaysInResponderChain) {
  /* If menuControllerStaysInResponderChain is false, we do not want to use
   * methods that might call setFirstResponder, because we might be in the
   * middle of another setFirstResponder call. */
  Script script =
      m_scriptStore->scriptAtIndex(m_selectableTableView.selectedRow());
  const char *scriptName = script.fullName();
  if (strlen(scriptName) <= 1 + strlen(ScriptStore::k_scriptExtension)) {
    // The previous text was an empty name. Use a numbered default script name.
    char numberedDefaultName[Script::k_defaultScriptNameMaxSize];
    bool foundDefaultName = Script::DefaultName(
        numberedDefaultName, Script::k_defaultScriptNameMaxSize);
    if (!foundDefaultName) {
      // If we did not find a default name, delete the script
      deleteScript(script);
      return true;
    }
    Script::ErrorStatus error = Ion::Storage::Record::SetBaseNameWithExtension(
        &script, numberedDefaultName, ScriptStore::k_scriptExtension);
    scriptName = script.fullName();
    /* Because we use the numbered default name, the name should not be
     * already taken. Plus, the script could be added only if the storage has
     * enough available space to add a script named 'script99.py' */
    // Silence the "variable unused" warning if assertions are not enabled
    (void)error;
    assert(error == Script::ErrorStatus::None);
    if (menuControllerStaysInResponderChain) {
      updateAddScriptRowDisplay();
    }
  }
  textField->setText(scriptName);
  if (menuControllerStaysInResponderChain) {
    m_selectableTableView.selectCellAtLocation(
        m_selectableTableView.selectedColumn(),
        m_selectableTableView.selectedRow());
    Container::activeApp()->setFirstResponder(&m_selectableTableView);
  }
  AppsContainer::sharedAppsContainer()->setShiftAlphaStatus(
      Ion::Events::ShiftAlphaStatus::Default);
  return true;
}

void MenuController::forceTextFieldEditionToAbort(
    bool menuControllerStaysInResponderChain) {
  int selectedRow = m_selectableTableView.selectedRow();
  int selectedColumn = m_selectableTableView.selectedColumn();
  if (selectedRow < 0 || selectedRow >= m_scriptStore->numberOfScripts() ||
      selectedColumn != 0) {
    return;
  }
  TextField *tf =
      static_cast<ScriptNameCell *>(m_selectableTableView.selectedCell())
          ->textField();
  if (!tf->isEditing()) {
    return;
  }
  tf->setEditing(false);
  privateTextFieldDidAbortEditing(tf, menuControllerStaysInResponderChain);
}

void MenuController::privateModalViewAltersFirstResponder(
    FirstResponderAlteration alteration) {
  if (alteration == FirstResponderAlteration::WillSpoil) {
    forceTextFieldEditionToAbort(true);
  }
}

}  // namespace Code
