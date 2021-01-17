#include <QVBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QHideEvent>
#include <QShowEvent>

#include <qt_window.h>
#include <recidia.h>

void StatsWidget::updateStats() {
    plotsCountLabel->setText("Plots: " + QString::number(recidia_data.plots_count));
    latencyLabel->setText("Latency: " + QString::number(recidia_data.latency, 'f', 1) + "ms");
    uint fps = (1000 / (recidia_data.frame_time / 1000)) + 0.5;
    fpsLabel->setText("FPS: " + QString::number(fps));
}

void StatsWidget::hideEvent(QHideEvent *event) {
    (void) event;
    timer->stop();
}

void StatsWidget::showEvent(QShowEvent *event) {
    (void) event;
    timer->start();
}

StatsWidget::StatsWidget() {
    // Make it not transparent
    this->setStyleSheet("background: palette(base)");

    QHBoxLayout *layout = new QHBoxLayout;
    this->setLayout(layout);

    plotsCountLabel = new QLabel("Plots: " + QString::number(recidia_data.plots_count), this);
    layout->addWidget(plotsCountLabel, 1);

    latencyLabel = new QLabel("Latency: " + QString::number(0) + "ms", this);
    layout->addWidget(latencyLabel, 1);

    fpsLabel = new QLabel("FPS: " + QString::number(0), this);
    layout->addWidget(fpsLabel, 1);

    QLabel *intervalLabel = new QLabel("Interval:", this);
    layout->addWidget(intervalLabel);
    QSpinBox *intervalSpinBox = new QSpinBox(this);
    intervalSpinBox->setFocusPolicy(Qt::ClickFocus);
    intervalSpinBox->setRange(1, 1000);
    intervalSpinBox->setValue(100);
    intervalSpinBox->setSuffix("ms");
    QObject::connect(intervalSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
    [=](int value) {
        timer->setInterval(value);
    });
    layout->addWidget(intervalSpinBox);

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &StatsWidget::updateStats);
    timer->setInterval(100);

    if(!recidia_settings.data.stats)
        this->hide();
    else
        timer->start();
}
