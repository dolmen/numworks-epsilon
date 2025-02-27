#ifndef FINANCE_APP_H
#define FINANCE_APP_H

#include <apps/i18n.h>
#include <escher/stack_view_controller.h>

#include "../shared/layout_field_delegate_app.h"
#include "../shared/shared_app.h"
#include "data.h"
#include "interest_menu_controller.h"
#include "menu_controller.h"
#include "parameters_controller.h"
#include "result_controller.h"

namespace Finance {

class App : public Shared::LayoutFieldDelegateApp {
 public:
  // Descriptor
  class Descriptor : public Shared::TextFieldDelegateApp::Descriptor {
   public:
    I18n::Message name() const override { return I18n::Message::FinanceApp; }
    I18n::Message upperName() const override {
      return I18n::Message::FinanceAppCapital;
    }
    const Escher::Image *icon() const override;
  };

  // Snapshot
  class Snapshot : public Shared::SharedApp::Snapshot {
   public:
    /* At most 3 nested menus from MenuController :
     * InterestMenuController, ParametersController and ResultController */
    constexpr static uint8_t k_maxNumberOfStacks = 3;

    App *unpack(Escher::Container *container) override;
    const Descriptor *descriptor() const override;
    void reset() override;
    Ion::RingBuffer<Escher::ViewController *, k_maxNumberOfStacks>
        *pageQueue() {
      return &m_pageQueue;
    }
    Data *data() { return &m_data; }

   private:
    Ion::RingBuffer<Escher::ViewController *, k_maxNumberOfStacks> m_pageQueue;
    Data m_data;
  };
  TELEMETRY_ID("Finance");

  static App *app() {
    return static_cast<App *>(Escher::Container::activeApp());
  }
  static InterestData *GetInterestData() {
    return app()->snapshot()->data()->interestData();
  }
  static void SetModel(bool selectedModel) {
    return app()->snapshot()->data()->setModel(selectedModel);
  }

  Snapshot *snapshot() const {
    return static_cast<Snapshot *>(Escher::App::snapshot());
  }

  void didBecomeActive(Escher::Window *window) override;

  // Navigation
  void willOpenPage(Escher::ViewController *controller) override;
  void didExitPage(Escher::ViewController *controller) override;

 private:
  App(Snapshot *snapshot);

  // Controllers
  ResultController m_resultController;
  ParametersController m_parametersController;
  InterestMenuController m_interestMenuController;
  MenuController m_menuController;
  Escher::StackViewController m_stackViewController;
};

}  // namespace Finance

#endif
