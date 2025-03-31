#include "mainwindow.hpp"
#include "../interface/scanner_impl.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QThread>
#include <QFuture>
#include <QtConcurrent>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , isScanning_(false)
{
    setupUi();
    createConnections();
}

void MainWindow::setupUi() {
    auto* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    auto* layout = new QVBoxLayout(centralWidget);

    // Directory selection
    auto* dirLayout = new QHBoxLayout();
    directoryEdit_ = new QLineEdit(this);
    browseButton_ = new QPushButton("Browse...", this);
    dirLayout->addWidget(new QLabel("Directory:"));
    dirLayout->addWidget(directoryEdit_);
    dirLayout->addWidget(browseButton_);
    layout->addLayout(dirLayout);

    // Options
    recursiveCheck_ = new QCheckBox("Scan recursively", this);
    recursiveCheck_->setChecked(true);
    layout->addWidget(recursiveCheck_);

    // Progress
    progressBar_ = new QProgressBar(this);
    layout->addWidget(progressBar_);

    // Buttons
    auto* buttonLayout = new QHBoxLayout();
    scanButton_ = new QPushButton("Start Scan", this);
    cancelButton_ = new QPushButton("Cancel", this);
    cancelButton_->setEnabled(false);
    buttonLayout->addWidget(scanButton_);
    buttonLayout->addWidget(cancelButton_);
    layout->addLayout(buttonLayout);

    // Results
    resultsEdit_ = new QTextEdit(this);
    resultsEdit_->setReadOnly(true);
    layout->addWidget(resultsEdit_);

    setWindowTitle("Dedupe++");
    resize(800, 600);
}

void MainWindow::createConnections() {
    connect(browseButton_, &QPushButton::clicked, this, &MainWindow::browseDirectory);
    connect(scanButton_, &QPushButton::clicked, this, &MainWindow::startScan);
    connect(cancelButton_, &QPushButton::clicked, this, &MainWindow::cancelScan);
}

void MainWindow::browseDirectory() {
    QString dir = QFileDialog::getExistingDirectory(this, "Select Directory",
                                                  directoryEdit_->text(),
                                                  QFileDialog::ShowDirsOnly);
    if (!dir.isEmpty()) {
        directoryEdit_->setText(dir);
    }
}

void MainWindow::startScan() {
    if (directoryEdit_->text().isEmpty()) {
        QMessageBox::warning(this, "Error", "Please select a directory first.");
        return;
    }

    isScanning_ = true;
    scanButton_->setEnabled(false);
    cancelButton_->setEnabled(true);
    resultsEdit_->clear();
    progressBar_->setValue(0);

    scanner_ = std::make_unique<dedupe::ScannerImpl>(recursiveCheck_->isChecked());

    // Start scanning in a separate thread
    QFuture<std::vector<dedupe::DuplicateGroup>> future = QtConcurrent::run(
        [this](const QString& dir, dedupe::IScanner* scanner) {
            return scanner->scan_directory(
                dir.toStdWString(),
                [this](const std::string& msg, double progress) {
                    QMetaObject::invokeMethod(this, "updateProgress",
                        Qt::QueuedConnection,
                        Q_ARG(QString, QString::fromStdString(msg)),
                        Q_ARG(double, progress));
                },
                [this]() { return !isScanning_; }
            );
        },
        directoryEdit_->text(),
        scanner_.get()
    );

    // Handle completion
    QFutureWatcher<std::vector<dedupe::DuplicateGroup>>* watcher = 
        new QFutureWatcher<std::vector<dedupe::DuplicateGroup>>(this);
    watcher->setFuture(future);
    connect(watcher, &QFutureWatcher<std::vector<dedupe::DuplicateGroup>>::finished,
            this, [this, watcher]() {
        scanCompleted(watcher->result());
        watcher->deleteLater();
    });
}

void MainWindow::cancelScan() {
    isScanning_ = false;
    scanButton_->setEnabled(true);
    cancelButton_->setEnabled(false);
}

void MainWindow::updateProgress(const QString& message, double progress) {
    progressBar_->setValue(static_cast<int>(progress * 100));
    resultsEdit_->setText(message);
}

void MainWindow::scanCompleted(const std::vector<dedupe::DuplicateGroup>& duplicates) {
    isScanning_ = false;
    scanButton_->setEnabled(true);
    cancelButton_->setEnabled(false);
    progressBar_->setValue(100);

    if (duplicates.empty()) {
        resultsEdit_->append("\nNo duplicate files found.");
        return;
    }

    resultsEdit_->append(QString("\nFound %1 groups of duplicate files:\n").arg(duplicates.size()));
    
    for (const auto& group : duplicates) {
        resultsEdit_->append(QString("\nHash: %1").arg(QString::fromStdString(group.hash)));
        for (const auto& file : group.files) {
            resultsEdit_->append(QString("  %1").arg(QString::fromStdWString(file.wstring())));
        }
    }
} 