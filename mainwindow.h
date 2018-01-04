#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QAbstractItemDelegate>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void on_module_new_clicked(bool check = false);
    void on_module_del_clicked(bool check = false);
    void on_command_new_clicked(bool check = false);
    void on_command_del_clicked(bool check = false);
    void on_submit_btn_clicked(bool check = false);
    void on_module_tbl_itemSelectionChanged();
    void on_command_tbl_itemSelectionChanged();

    void command_tbl_closeEditor(QWidget *editor, QAbstractItemDelegate::EndEditHint hint);
private:
    void update_command_view( QString &cmd );
private:
    Ui::MainWindow *ui;

    QString _module_select;
    QString _command_slect;
};

#endif // MAINWINDOW_H
