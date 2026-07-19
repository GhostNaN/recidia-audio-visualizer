#include <string>
#include <vector>
#include <unistd.h>

#include <QApplication>
#include <QDialog>
#include <QListWidget>
#include <QVBoxLayout>

#include <qt_window.hpp>

using namespace std;

int display_audio_devices(vector<string> devices, vector<uint> pipe_indexes) {
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

    QVBoxLayout *layout = new QVBoxLayout;
    dialog->setLayout(layout);

    uint i = 0;

    QLabel *pipeLabel = new QLabel("Pipewire Nodes", dialog);
    pipeLabel->setAlignment(Qt::AlignHCenter);
    layout->addWidget(pipeLabel);

    layout->addWidget(pipeList);
    for (i=0; i < pipe_indexes.size(); i++) {
        pipeList->addItem(QString::fromStdString(devices[i]));
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

    int index = pipeList->currentRow();

    return index;
}
 