// mainwindow.h
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTreeWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <yaml-cpp/yaml.h>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onSaveButtonClicked();
    void showContextMenu(const QPoint &pos);

private:
    Ui::MainWindow *ui;
    QTreeWidget *treeWidget;
    QPushButton *saveButton;

    void parseYamlToTree(const QString &filePath);
    void addYamlNodeToTree(const YAML::Node &node, QTreeWidgetItem *parentItem);
    void addTreeNodeToYaml(QTreeWidgetItem *item, YAML::Node &parentNode);
    void editSequenceNode(QTreeWidgetItem *item);
    void collectFieldsFromItem(QTreeWidgetItem *item, QMap<QString, QString> &fieldMap);
    void saveTreeToYaml(const QString &filePath);
    void editLeafValue(QTreeWidgetItem *item);
    void deleteSequenceItem(QTreeWidgetItem *item);
    void mapStructure(QTreeWidgetItem *item, QMap<QString, QTreeWidgetItem*> &structure, QString parentPath);
    void createTreePath(QTreeWidgetItem *parent, const QString &path, const QString &value);
};

#endif // MAINWINDOW_H
