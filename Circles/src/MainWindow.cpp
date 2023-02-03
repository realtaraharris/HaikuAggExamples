#include <iostream>

#include <Application.h>
#include <Menu.h>
#include <MenuItem.h>
#include <View.h>
#include <Alert.h> // BAlert
#include "App.h" // contains message enums

#include "MainWindow.h"
#include "AGGView.h"

MainWindow::MainWindow(void)
    : BWindow(BRect(100, 100, 600, 600), "Main Window", B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS, B_CURRENT_WORKSPACE) {
    BRect r(Bounds());
    r.bottom = 20;
    menuBar = new BMenuBar(r, "menubar");
    AddChild(menuBar);

    BMenu *menu = new BMenu("File");
    menu->AddItem(new BMenuItem("Quit", new BMessage(M_QUIT_APP), 'Q'));
    menuBar->AddItem(menu);

    r.Set(0, 40, 500, 500);
    aggView = new AGGView(r);
    AddChild(aggView);

    r.Set(5, 40, 255, 55);
    sizeSlider = new BSlider(r, NULL, "Size", new BMessage(SIZE_SLIDER), 0, 1);
    AddChild(sizeSlider);

    r.Set(5, 80, 255, 55);
    selectivitySlider = new BSlider(r, NULL, "Selectivity", new BMessage(SELECTIVITY_SLIDER), 0, 1);
    AddChild(selectivitySlider);
}

void MainWindow::MessageReceived(BMessage *msg) {
    switch (msg->what) {
        case SIZE_SLIDER: {
            break;
        }
        case SELECTIVITY_SLIDER: {
            break;
        }
        case M_QUIT_APP: {
            // this won't work because you need to dispatch another message, say REALLY_QUIT to avoid triggering the alert...
            // BAlert *alert = new BAlert("Quit?", "Are you sure you want to quit?", "OK", "Cancel", NULL, B_WIDTH_AS_USUAL, B_INFO_ALERT);
            // alert->Go(NULL);
            be_app->PostMessage(B_QUIT_REQUESTED);
            break;
        }
        default: {
            std::cout << "Message received: " << msg->what << std::endl;

            BWindow::MessageReceived(msg);
            break;
        }
    }
}

bool MainWindow::QuitRequested(void) {
    be_app->PostMessage(B_QUIT_REQUESTED);
    return true;
}