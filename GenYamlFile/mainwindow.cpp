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
#include <QGroupBox>
#include <QFormLayout>
#include <QScrollArea>
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

    // Thêm nút Delete cho các item trong sequence
    if (item->text(0).startsWith("Item") &&
        item->parent() &&
        isSequenceNode(item->parent())) {
        QAction *deleteAction = contextMenu.addAction("Delete");
        connect(deleteAction, &QAction::triggered, this, [this, item]() {
            deleteSequenceItem(item);
        });
    }

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

void MainWindow::deleteSequenceItem(QTreeWidgetItem *item)
{
    if (!item || !item->parent()) return;

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Confirm Deletion",
        "Are you sure you want to delete this item?",
        QMessageBox::Yes | QMessageBox::No
        );

    if (reply == QMessageBox::Yes) {
        // Lấy parent để cập nhật lại số thứ tự sau khi xóa
        QTreeWidgetItem *parent = item->parent();
        int removedIndex = parent->indexOfChild(item);
        delete item;

        // Cập nhật lại số thứ tự của các items còn lại
        for (int i = removedIndex; i < parent->childCount(); i++) {
            parent->child(i)->setText(0, QString("Item %1").arg(i));
        }
    }
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

    // Get structure from first child if exists
    QMap<QString, QTreeWidgetItem*> structure;
    if (item->childCount() > 0) {
        mapStructure(item->child(0), structure, "/");
    }

    QDialog dialog(this);
    dialog.setWindowTitle("Edit Sequence");
    QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);
    QScrollArea *scrollArea = new QScrollArea(&dialog);
    QWidget *scrollContent = new QWidget;
    QVBoxLayout *contentLayout = new QVBoxLayout(scrollContent);

    // Map to store all input fields
    QMap<QString, QWidget*> inputFields;

    // Create inputs based on structure
    for (auto it = structure.begin(); it != structure.end(); ++it) {
        QString path = it.key();
        QTreeWidgetItem* structureItem = it.value();

        if (structureItem->childCount() > 0) {
            // Create group for nested structure
            QGroupBox *group = new QGroupBox(path, &dialog);
            QFormLayout *groupLayout = new QFormLayout(group);

            // Create fields for children
            for (int i = 0; i < structureItem->childCount(); i++) {
                QTreeWidgetItem* child = structureItem->child(i);
                QString childPath = path + "/" + child->text(0);
                QLineEdit *input = new QLineEdit(group);
                groupLayout->addRow(child->text(0), input);
                inputFields[childPath] = input;
            }

            contentLayout->addWidget(group);
        } else {
            // Create field for leaf node
            QLineEdit *input = new QLineEdit(scrollContent);
            QFormLayout *fieldLayout = new QFormLayout;
            fieldLayout->addRow(path, input);
            contentLayout->addLayout(fieldLayout);
            inputFields[path] = input;
        }
    }

    scrollArea->setWidget(scrollContent);
    scrollArea->setWidgetResizable(true);
    mainLayout->addWidget(scrollArea);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        Qt::Horizontal,
        &dialog
        );
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    if (dialog.exec() == QDialog::Accepted) {
        QTreeWidgetItem *newItem = new QTreeWidgetItem(item);
        newItem->setText(0, QString("Item %1").arg(item->childCount() - 1));

        // Create tree structure based on input values
        for (auto it = structure.begin(); it != structure.end(); ++it) {
            QString path = it.key();
            QWidget *input = inputFields[path];

            if (QLineEdit *lineEdit = qobject_cast<QLineEdit*>(input)) {
                if (!lineEdit->text().isEmpty()) {
                    createTreePath(newItem, path, lineEdit->text());
                }
            }
        }
    }
}

void MainWindow::mapStructure(QTreeWidgetItem *item, QMap<QString, QTreeWidgetItem*> &structure, QString parentPath)
{
    QString currentPath = parentPath.isEmpty() ? item->text(0) : parentPath + "/" + item->text(0);
    structure[currentPath] = item;

    for (int i = 0; i < item->childCount(); i++) {
        mapStructure(item->child(i), structure, currentPath);
    }
}

void MainWindow::createTreePath(QTreeWidgetItem *parent, const QString &path, const QString &value)
{
    QStringList parts = path.split("/");
    QTreeWidgetItem *current = parent;

    for (int i = 0; i < parts.size(); i++) {
        QTreeWidgetItem *child = nullptr;
        QString part = parts[i];

        // Check if node already exists
        for (int j = 0; j < current->childCount(); j++) {
            if (current->child(j)->text(0) == part) {
                child = current->child(j);
                break;
            }
        }

        if (!child) {
            child = new QTreeWidgetItem(current);
            child->setText(0, part);
        }

        if (i == parts.size() - 1) {
            child->setText(1, value);
        }

        current = child;
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
