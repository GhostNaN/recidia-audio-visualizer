#include <string>
#include <vector>
#include <unistd.h>

#include <QApplication>
#include <QDialog>
#include <QListWidget>
#include <QVBoxLayout>

#include <qt_window.hpp>

using namespace std;

int display_audio_devices(vector<string> devices, vector<uint> pipe_indexes, vector<uint> pulse_indexes, vector<uint> port_indexes) {
    const char *argv[] = {"", NULL};
    int argc = 1;
    QApplication app(argc, const_cast<char **>(argv));

    QDialog *dialog = new QDialog;

    QObject::connect(dialog, &QDialog::accepted,
    [=]() { dialog->close(); } );
    QObject::connect(dialog, &QDialog::rejected,
    [=]() { exit(0); } );

    dialog->setWindowTitle("Select Audio Device");

    QListWidget *pipeList = new QListWidget(dialog);
    QListWidget *pulseList = new QListWidget(dialog);
    QListWidget *portList = new QListWidget(dialog);

    // Make sure only 1 device is selected
    QObject::connect(pipeList, &QListWidget::currentRowChanged,
    [=]() { portList->clearSelection();} );

    QObject::connect(pulseList, &QListWidget::currentRowChanged,
    [=]() { portList->clearSelection();} );

    QObject::connect(portList, &QListWidget::currentRowChanged,
    [=]() { pipeList->clearSelection(); pulseList->clearSelection();} );
    

    QVBoxLayout *layout = new QVBoxLayout;
    dialog->setLayout(layout);

    uint i = 0;

    if (pipe_indexes.size()) {
        QLabel *pipeLabel = new QLabel("Pipewire Nodes", dialog);
        pipeLabel->setAlignment(Qt::AlignHCenter);
        layout->addWidget(pipeLabel);

        layout->addWidget(pipeList);
        for (i=0; i < pipe_indexes.size(); i++) {
            pipeList->addItem(QString::fromStdString(devices[i]));
        }
    }
    else if (pulse_indexes.size()) {
        QLabel *pulseLabel = new QLabel("PulseAudio Devices", dialog);
        pulseLabel->setAlignment(Qt::AlignHCenter);
        layout->addWidget(pulseLabel);

        layout->addWidget(pulseList);
        pulseList->addItem("Default Output");
        for (i=0; i < pulse_indexes.size(); i++) {
            pulseList->addItem(QString::fromStdString(devices[i]));
        }
    }

    if (port_indexes.size()) {
        QLabel *portLabel = new QLabel("PortAudio Devices", dialog);
        portLabel->setAlignment(Qt::AlignHCenter);
        layout->addWidget(portLabel);

        layout->addWidget(portList);
        for (uint j=0; j < port_indexes.size(); j++) {
            portList->addItem(QString::fromStdString(devices[i+j]));
        }
    }

    QHBoxLayout *buttonLayout = new QHBoxLayout;

    QPushButton *okButton = new QPushButton("Ok", dialog);
    QObject::connect(okButton, &QPushButton::released,
    [=]() { dialog->accept(); } );
    buttonLayout->addWidget(okButton);

    QPushButton *cancelButton = new QPushButton("Cancel", dialog);
    QObject::connect(cancelButton, &QPushButton::released,
    [=]() { dialog->reject(); } );
    buttonLayout->addWidget(cancelButton);

    layout->addLayout(buttonLayout);

    dialog->resize(600, 450);
    dialog->exec();


    int index = 0;
    if  (pipeList->selectedItems().size() != 0)
        index = pipeList->currentRow();
    else if (pulseList->selectedItems().size() != 0)
        index = pulseList->currentRow();
    else if (portList->selectedItems().size() != 0) {
        index = portList->currentRow();
        if (pipe_indexes.size())
            index += pipe_indexes.size();
        else if (pulse_indexes.size())
            index += pulse_indexes.size()+1;
    }

    return index;
}
 