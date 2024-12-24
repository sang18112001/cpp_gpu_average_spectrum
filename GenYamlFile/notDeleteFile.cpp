// mainwindow.cpp
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFile>
#include <QInputDialog>
#include <QMessageBox>
#include <QMenu>
#include <QDebug>
#include <fstream>
#include <QTableWidget>
#include <QDialogButtonBox>
#include <QHeaderView>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Create tree widget
    treeWidget = new QTreeWidget(this);
    treeWidget->setColumnCount(2);
    treeWidget->setHeaderLabels(QStringList() << "Key" << "Value");

    // Create save button
    saveButton = new QPushButton("Save YAML", this);
    connect(saveButton, &QPushButton::clicked, this, &MainWindow::onSaveButtonClicked);

    // Create layout
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(treeWidget);
    layout->addWidget(saveButton);
    QWidget *centralWidget = new QWidget(this);
    centralWidget->setLayout(layout);
    setCentralWidget(centralWidget);

    // Load and parse YAML file
    QString yamlFilePath = "/home/sang/Documents/QtCpp/GenYamlFile/config.yaml"; // Update this to your YAML file path
    parseYamlToTree(yamlFilePath);

    // Context menu
    treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(treeWidget, &QTreeWidget::customContextMenuRequested, this, &MainWindow::showContextMenu);
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool isLeafNode(QTreeWidgetItem *item)
{
    return item->childCount() == 0; // A leaf node has no children
}

bool isSequenceNode(QTreeWidgetItem *item)
{
    // Check if the item represents a sequence (e.g., array)
    for (int i = 0; i < item->childCount(); ++i) {
        if (item->child(i)->text(0).startsWith("Item")) {
            return true;
        }
    }
    return false;
}

void MainWindow::parseYamlToTree(const QString &filePath)
{
    try {
        YAML::Node root = YAML::LoadFile(filePath.toStdString());
        addYamlNodeToTree(root, nullptr);
    } catch (const YAML::Exception &e) {
        qDebug() << "Failed to load YAML file:" << e.what();
    }
}

void MainWindow::addYamlNodeToTree(const YAML::Node &node, QTreeWidgetItem *parentItem)
{
    if (node.IsMap()) {
        for (const auto &it : node) {
            QString key = QString::fromStdString(it.first.as<std::string>());
            QTreeWidgetItem *item = parentItem ? new QTreeWidgetItem(parentItem) : new QTreeWidgetItem(treeWidget);
            item->setText(0, key);

            if (it.second.IsScalar()) {
                item->setText(1, QString::fromStdString(it.second.as<std::string>()));
            } else {
                addYamlNodeToTree(it.second, item);
            }
        }
    } else if (node.IsSequence()) {
        for (std::size_t i = 0; i < node.size(); ++i) {
            QString key = QString("Item %1").arg(i);
            QTreeWidgetItem *item = parentItem ? new QTreeWidgetItem(parentItem) : new QTreeWidgetItem(treeWidget);
            item->setText(0, key);

            if (node[i].IsScalar()) {
                item->setText(1, QString::fromStdString(node[i].as<std::string>()));
            } else {
                addYamlNodeToTree(node[i], item);
            }
        }
    }
}

void MainWindow::showContextMenu(const QPoint &pos)
{
    QTreeWidgetItem *item = treeWidget->itemAt(pos);
    if (!item) return;

    QMenu contextMenu(this);

    if (isSequenceNode(item)) {
        QAction *editSequenceAction = contextMenu.addAction("Edit Sequence");
        connect(editSequenceAction, &QAction::triggered, this, [this, item]() {
            editSequenceNode(item);
        });
    }

    if (isLeafNode(item)) {
        QAction *editValueAction = contextMenu.addAction("Edit Value");
        connect(editValueAction, &QAction::triggered, this, [this, item]() {
            editLeafValue(item);
        });
    }

    contextMenu.exec(treeWidget->mapToGlobal(pos));
}

void MainWindow::editLeafValue(QTreeWidgetItem *item)
{
    if (!item) return;

    bool ok;
    QString currentValue = item->text(1);
    QString newValue = QInputDialog::getText(this, "Edit Value", "Enter new value:", QLineEdit::Normal, currentValue, &ok);

    if (ok && !newValue.isEmpty()) {
        item->setText(1, newValue);
    }
}

void MainWindow::editSequenceNode(QTreeWidgetItem *item)
{
    if (!item) return;

    // Collect fields for the new object
    QMap<QString, QString> fieldMap;
    if (item->childCount() > 0) {
        QTreeWidgetItem *firstChild = item->child(0);
        for (int i = 0; i < firstChild->childCount(); ++i) {
            QTreeWidgetItem *fieldItem = firstChild->child(i);
            fieldMap[fieldItem->text(0)] = ""; // Initialize fields with empty values
        }
    }

    // Create a dialog for editing sequence
    QDialog dialog(this);
    dialog.setWindowTitle("Edit Sequence");
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);

    QTableWidget *tableWidget = new QTableWidget(fieldMap.size(), 2, &dialog);
    tableWidget->setHorizontalHeaderLabels(QStringList() << "Key" << "Value");
    tableWidget->horizontalHeader()->setStretchLastSection(true);
    tableWidget->verticalHeader()->setVisible(false);
    tableWidget->setEditTriggers(QAbstractItemView::AllEditTriggers);

    int row = 0;
    for (auto it = fieldMap.begin(); it != fieldMap.end(); ++it, ++row) {
        QTableWidgetItem *keyItem = new QTableWidgetItem(it.key());
        keyItem->setFlags(keyItem->flags() & ~Qt::ItemIsEditable); // Make key column read-only
        tableWidget->setItem(row, 0, keyItem);

        QTableWidgetItem *valueItem = new QTableWidgetItem(it.value());
        tableWidget->setItem(row, 1, valueItem);
    }

    dialogLayout->addWidget(tableWidget);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    dialogLayout->addWidget(buttonBox);

    if (dialog.exec() == QDialog::Accepted) {
        // Collect values entered by the user
        QMap<QString, QString> newFields;
        for (int i = 0; i < tableWidget->rowCount(); ++i) {
            QString key = tableWidget->item(i, 0)->text();
            QString value = tableWidget->item(i, 1)->text();
            if (!value.isEmpty()) {
                newFields[key] = value;
            }
        }

        // Add new object to the sequence
        QTreeWidgetItem *newItem = new QTreeWidgetItem(item);
        newItem->setText(0, QString("Item %1").arg(item->childCount() - 1));
        for (auto it = newFields.begin(); it != newFields.end(); ++it) {
            QTreeWidgetItem *fieldItem = new QTreeWidgetItem(newItem);
            fieldItem->setText(0, it.key());
            fieldItem->setText(1, it.value());
        }
    }
}

void MainWindow::onSaveButtonClicked()
{
    QString filePath = "/home/sang/Documents/QtCpp/GenYamlFile/output.yaml"; // Update to your desired output path
    saveTreeToYaml(filePath);
}

void MainWindow::saveTreeToYaml(const QString &filePath)
{
    YAML::Node root;

    for (int i = 0; i < treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = treeWidget->topLevelItem(i);
        addTreeNodeToYaml(item, root);
    }

    try {
        std::ofstream outFile(filePath.toStdString());
        outFile << root;
        outFile.close();
        qDebug() << "YAML file saved to:" << filePath;
        QMessageBox::information(this, "Success", "YAML file saved successfully.");
    } catch (const std::exception &e) {
        qDebug() << "Failed to save YAML file:" << e.what();
        QMessageBox::critical(this, "Error", "Failed to save YAML file.");
    }
}

void MainWindow::addTreeNodeToYaml(QTreeWidgetItem *item, YAML::Node &parentNode)
{
    QString key = item->text(0);
    QString value = item->text(1);

    if (key.startsWith("Item")) { // Handle sequence
        YAML::Node subNode;
        for (int i = 0; i < item->childCount(); ++i) {
            QTreeWidgetItem *childItem = item->child(i);
            addTreeNodeToYaml(childItem, subNode);
        }
        parentNode.push_back(subNode);
    } else if (item->childCount() > 0) { // Handle map
        YAML::Node subNode;
        for (int i = 0; i < item->childCount(); ++i) {
            QTreeWidgetItem *childItem = item->child(i);
            addTreeNodeToYaml(childItem, subNode);
        }
        parentNode[key.toStdString()] = subNode;
    } else { // Handle scalar
        parentNode[key.toStdString()] = value.toStdString();
    }
}
