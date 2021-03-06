#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "proto.h"
#include "config.h"

#include <QTimer>
#include <QProcess>
#include <QMessageBox>

#define DEF_FIELD "undefine"
#define MAIN_TITLE "Stream Protocol Editor"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    _search_dialog = new QSearchDialog( this );

    class config *conf = config::instance();

    const QList<QString> &module_field = conf->get_module_field();
    ui->module_tbl->setColumnCount( module_field.length() );
    ui->module_tbl->setHorizontalHeaderLabels( module_field );

    ui->module_tbl->verticalHeader()->setVisible( false );
    ui->module_tbl->setSelectionBehavior( QAbstractItemView::SelectItems );
    ui->module_tbl->setSelectionMode( QAbstractItemView::SingleSelection );

    const QList<QString> &command_field = conf->get_command_field();
    ui->command_tbl->setColumnCount( command_field.length() );
    ui->command_tbl->setHorizontalHeaderLabels( conf->get_command_field() );
//    ui->command_tbl->horizontalHeader()
//            ->setStyleSheet("QHeaderView::section{background:skyblue;}"); //设置表头背景色

    ui->command_tbl->verticalHeader()->setVisible( false );
    ui->command_tbl->setSelectionBehavior( QAbstractItemView::SelectItems );
    ui->command_tbl->setSelectionMode( QAbstractItemView::SingleSelection );

    // AUTO-CONNECTION:
    // http://doc.qt.io/qt-5/designer-using-a-ui-file.html#widgets-and-dialogs-with-auto-connect
    // connect( ui->module_new,SIGNAL(clicked(bool)),this,SLOT(on_module_new(bool)) );

    connect( ui->module_tbl->itemDelegate(),
         SIGNAL(commitData(QWidget*)),
         this,
         SLOT(module_tbl_commit_data(QWidget*))
     );

    connect( ui->command_tbl->itemDelegate(),
         SIGNAL(commitData(QWidget*)),
         this,
         SLOT(command_tbl_commit_data(QWidget*))
     );

    connect(_search_dialog,
            SIGNAL(result_double_click(QString,QString,QString)),
            this,SLOT(show_search_result(QString,QString,QString)));

    class proto *pt = proto::instance();
    bool ok = pt->load(
                conf->get_source_path(),conf->get_module_key(),conf->get_command_key() );
    if ( ok )
    {
        update_module_view();
    }
    else
    {
        ui->statusBar->showMessage( pt->get_error_text(),0 );
    }

    QTimer *save_timer = new QTimer(this);
    connect( save_timer,SIGNAL(timeout()),this,SLOT(save_proto()) );

    save_timer->start( 5000 );
}

MainWindow::~MainWindow()
{
    const QString &path = config::instance()->get_source_path();
    proto::instance()->save( path );
    delete ui;
    delete _search_dialog;
}

void MainWindow::on_module_new_clicked(bool check)
{
    Q_UNUSED(check);

    // 由于采用了双击编辑，很难确定用户什么时候编辑完成一个module的各个子项
    // 因此，在创新module时，自动填充所有子项。以后的编辑，均采用更新方式
    class config *conf = config::instance();
    const QList<QString> &module_field = conf->get_module_field();

    Fields fields;
    for ( int idx = 0;idx < module_field.length();idx ++ )
    {
        fields[module_field.at(idx)] = DEF_FIELD;
    }
    // 防止用户拼命点添加
    if ( !proto::instance()->new_module(DEF_FIELD,fields) )
    {
        QMessageBox box;
        box.setText( "dumplicate module" );
        box.exec();
        return;
    }

    int rows = ui->module_tbl->rowCount();
    ui->module_tbl->setRowCount( rows + 1 );
    for ( int idx = 0;idx < module_field.length();idx ++ )
    {
        QTableWidgetItem *item = new QTableWidgetItem(DEF_FIELD);
        item->setData(Qt::UserRole,DEF_FIELD);
        ui->module_tbl->setItem( rows,idx,item );
    }
}

void MainWindow::on_module_del_clicked(bool check)
{
    Q_UNUSED(check);

    int row = ui->module_tbl->currentRow();
    if ( row < 0 ) return;  /* no select */

    const QTableWidgetItem *item = ui->module_tbl->item( row,0 );
    QString cmd = item->text();

    ui->module_tbl->removeRow( row );
    proto::instance()->del_module( cmd );
}

void MainWindow::on_command_new_clicked(bool check)
{
    Q_UNUSED(check);

    int row = ui->module_tbl->currentRow();
    if ( row < 0 )
    {
        QMessageBox box;
        box.setText( "no module slected" );
        box.exec();
        return;
    }

    const QTableWidgetItem *item = ui->module_tbl->item( row,0 );
    QString module_cmd = item->text();

    class config *conf = config::instance();
    const QList<QString> &command_field = conf->get_command_field();

    Fields fields;
    for ( int idx =0;idx < command_field.length();idx ++ )
    {
        fields[command_field.at(idx)] = DEF_FIELD;
    }

    if ( !proto::instance()->new_command(module_cmd,DEF_FIELD,fields) )
    {
        QMessageBox box;
        box.setText( "no module selected or dumplicate command" );
        box.exec();
        return;
    }

    update_command_view( module_cmd);
}

void MainWindow::on_command_del_clicked(bool check)
{
    Q_UNUSED(check);

    int row = ui->module_tbl->currentRow();
    if ( row < 0 )
    {
        QMessageBox box;
        box.setText( "no module slected" );
        box.exec();
        return;
    }

    int cmd_row = ui->module_tbl->currentRow();
    if ( cmd_row < 0 )
    {
        QMessageBox box;
        box.setText( "no command slected" );
        box.exec();
        return;
    }


    const QTableWidgetItem *item = ui->module_tbl->item( row,0 );
    const QTableWidgetItem *cmd_item = ui->command_tbl->item( cmd_row,0 );

    proto::instance()->del_command( item->text(),cmd_item->text() );
    ui->command_tbl->removeRow( cmd_row );
}

void MainWindow::update_module_view()
{
    ui->module_tbl->clearContents();
    const QList<const Fields*> list = proto::instance()->get_module();
    if ( list.length() <= 0 ) return;

    class config *conf = config::instance();
    const QList<QString> &module_field = conf->get_module_field();

    ui->module_tbl->setRowCount( list.length() );
    for ( int idx = 0;idx < list.length();idx ++ )
    {
        const Fields *fields = list.at( idx );
        if ( !fields ) continue;

        for ( int field_idx = 0;field_idx < module_field.length();field_idx ++ )
        {
            Fields::ConstIterator itr = fields->find( module_field.at(field_idx) );
            if ( itr != fields->constEnd() )
            {
                QTableWidgetItem *item = new QTableWidgetItem(*itr);
                item->setData(Qt::UserRole,*itr);
                ui->module_tbl->setItem( idx,field_idx,item );
            }
        }
    }
}

void MainWindow::update_command_view( QString &cmd )
{
    ui->command_tbl->clearContents();
    const CmdMap *cmd_map = proto::instance()->get_module_cmd( cmd );
    if ( !cmd_map ) return;

    class config *conf = config::instance();
    const QList<QString> &command_field = conf->get_command_field();

    int rows = 0;
    ui->command_tbl->setRowCount( cmd_map->size() );
    for ( CmdMap::ConstIterator itr = cmd_map->constBegin();itr != cmd_map->constEnd();itr ++ )
    {
        const Fields &fields = *(itr);
        for ( int idx = 0;idx < command_field.length();idx ++ )
        {
            Fields::ConstIterator cmd_itr = fields.find( command_field.at(idx) );
            if ( cmd_itr != fields.constEnd() )
            {
                QTableWidgetItem *item = new QTableWidgetItem(*cmd_itr);
                item->setData(Qt::UserRole,*cmd_itr);
                ui->command_tbl->setItem( rows,idx,item );
            }
        }
        rows ++;
    }
}

void MainWindow::on_module_tbl_itemSelectionChanged()
{
    _module_select.clear();
    _command_slect.clear();
    ui->statusBar->clearMessage();

    int row = ui->module_tbl->currentRow();
    if ( row < 0 ) return;

    const QTableWidgetItem *item = ui->module_tbl->item( row,0 );

    _module_select = item->text();
    QString field = "?";
    QList<QTableWidgetItem *> select_item = ui->module_tbl->selectedItems();
    if ( select_item.length() > 0 )
    {
        // as module_tbl is set in single select mode,only one item should be selected
        ui->detail_edt->setPlainText( select_item.at(0)->text() );
        int column = select_item.at(0)->column();

        const QList<QString> &module_field = config::instance()->get_module_field();
        if ( column >= 0 && column < module_field.length() )
        {
            field = module_field.at(column);
        }
    }

    update_command_view( _module_select );
    ui->statusBar->showMessage( QString("select:%1-%2").arg(_module_select).arg(field) );
}

void MainWindow::on_command_tbl_itemSelectionChanged()
{
    ui->statusBar->clearMessage();

    int row = ui->module_tbl->currentRow();
    if ( row < 0 ) return;

    const QTableWidgetItem *item = ui->module_tbl->item( row,0 );

    int cmd_row = ui->command_tbl->currentRow();
    if ( cmd_row < 0 ) return;

    const QTableWidgetItem *cmd_item = ui->command_tbl->item( cmd_row,0 );
    _command_slect = cmd_item->text();

    QString field = "?";
    QList<QTableWidgetItem *> select_item = ui->command_tbl->selectedItems();
    if ( select_item.length() > 0 )
    {
        // as module_tbl is set in single select mode,only one item should be selected
        ui->detail_edt->setPlainText( select_item.at(0)->text() );

        int column = select_item.at(0)->column();

        const QList<QString> &command_field = config::instance()->get_command_field();
        if ( column >= 0 && column < command_field.length() )
        {
            field = command_field.at(column);
        }
    }

    ui->statusBar->showMessage(
                QString("select:%1-%2-%3").arg(
                item->text()).arg(cmd_item->text()).arg(field) );
}

QTableWidgetItem *MainWindow::get_select_item(QTableWidget *widget)
{
    QList<QTableWidgetItem *> select_item = widget->selectedItems();
    if ( select_item.length() <= 0 ) return NULL;

    return select_item.at(0);
}

bool MainWindow::raw_update_module(QTableWidgetItem *item, const QString &ctx)
{
    int column = item->column();

    const QList<QString> &module_field = config::instance()->get_module_field();
    if ( column >= 0 && column < module_field.length() )
    {
        // 可能修改的就是module的key
        bool ok = proto::instance()->update_module(
            _module_select,module_field.at(column),ctx,0 == column );

        return ok;
    }

    return false;
}

bool MainWindow::raw_update_command(QTableWidgetItem *item,const QString &ctx)
{
    int column = item->column();

    const QList<QString> &command_field = config::instance()->get_command_field();
    if ( column >= 0 && column < command_field.length() )
    {
        // 可能修改的就是module的key
        bool ok = proto::instance()->update_command(
            _module_select,_command_slect,command_field.at(column),ctx,0 == column );

        return ok;
    }

    return false;
}

void MainWindow::on_submit_btn_clicked( bool check )
{
    Q_UNUSED(check);
    if ( _module_select.isEmpty() ) return;

    bool is_command = !_command_slect.isEmpty();
    QTableWidgetItem *item = is_command ?
                get_select_item( ui->command_tbl ) : get_select_item( ui->module_tbl );
    if ( !item ) return;

    const QString &ctx = ui->detail_edt->toPlainText();
    bool ok = is_command ?
                raw_update_command( item,ctx ) : raw_update_module( item,ctx );
    if ( !ok )
    {
        QMessageBox box;
        box.setText( proto::instance()->get_error_text() );
        box.exec();

        return;
    }

    item->setText( ctx );
}

void MainWindow::module_tbl_commit_data(QWidget *editor)
{
    Q_UNUSED(editor);

    QTableWidgetItem *item = get_select_item( ui->module_tbl );
    if ( !item ) return;

    bool ok = raw_update_module( item,item->text() );
    if ( !ok )
    {
        // restore old data
        item->setText( item->data(Qt::UserRole).toString() );
    }
}

void MainWindow::command_tbl_commit_data(QWidget *editor)
{
    Q_UNUSED(editor);

    QTableWidgetItem *item = get_select_item( ui->command_tbl );
    if ( !item ) return;

    bool ok = raw_update_command( item,item->text() );
    if ( !ok )
    {
        // restore old data
        item->setText( item->data(Qt::UserRole).toString() );
    }
}

void MainWindow::on_action_about_triggered()
{
    // support html
   QMessageBox::about(this, tr(MAIN_TITLE),
            tr("A custom protocol editor<br/>"
               "<b>USAGE</b><br/>"
               "1. the program will generate setting example file(setting.int) at first time running<br/>"
               "2. custom your module and command field in setting.init<br/>"
               )
   );
}

void MainWindow::on_action_export_triggered()
{
    const QString &cmd = config::instance()->get_export_command();
    bool succes = QProcess::startDetached( cmd );
    if ( !succes )
    {
        QMessageBox box;
        box.setText( QString("can not run command:%1").arg(cmd) );
        box.exec();
        return;
    }
}

void MainWindow::on_search_btn_clicked(bool check)
{
    Q_UNUSED(check);

    const QString &ctx = ui->search_edt->text();
    if ( ctx.isEmpty() ) return;

    if ( !_search_dialog )
    {
        _search_dialog = new QSearchDialog(this );
    }

    QList<search_ctx> list;

    do_search( ctx,list );
    _search_dialog->update_content( ctx,list );
}

bool search_fields(const Fields &fields,const QString &ctx,QString &key,QString &val)
{
    Fields::ConstIterator itr = fields.constBegin();
    for ( ;itr != fields.constEnd();itr ++ )
    {
        const QString &value = itr.value();
        if ( value.contains(ctx,Qt::CaseInsensitive) )
        {
            key = itr.key();
            val = value;
            return true;
        }
    }
    return false;
}

/* 搜索规则：
 * 1. 优先搜索command
 * 2. 同一个module或command中不同field中出现关键字时，只显示第一个field
 */
void MainWindow::do_search( const QString &ctx,QList<search_ctx> &list )
{
    const class proto *pto = proto::instance();
    const class config *conf = config::instance();

    const QList<const Fields*> module_list = proto::instance()->get_module();
    if ( module_list.length() <= 0 ) return;

    const QString &module_key = conf->get_module_key();

    for ( int idx = 0;idx < module_list.length();idx ++)
    {
        QString key;
        QString val;
        const Fields *module_fields = module_list.at( idx );
        Fields::ConstIterator module_itr = module_fields->find( module_key );
        if ( module_itr == module_fields->constEnd() ) continue; // should not happen

        const CmdMap *cmd_map = pto->get_module_cmd( *module_itr );
        if ( cmd_map ) // 搜索command
        {
            CmdMap::ConstIterator cmd_itr = cmd_map->constBegin();
            for ( ;cmd_itr != cmd_map->constEnd();cmd_itr ++ )
            {
                if ( search_fields(*cmd_itr,ctx,key,val) )
                {
                    struct search_ctx sctx;
                    sctx._module = *module_itr;
                    sctx._command = cmd_itr.key();
                    sctx._field   = key;
                    sctx._content = val;
                    list.append( sctx );
                }
            }
        }

        if ( search_fields(*module_fields,ctx,key,val) )
        {
            struct search_ctx sctx;
            sctx._module = *module_itr;
            sctx._command = "";
            sctx._field   = key;
            sctx._content = val;
            list.append( sctx );
        }
    }
}

void MainWindow::show_search_result(
    const QString &module,const QString &command,const QString &field)
{
    const config *conf = config::instance();
    //  找出module_key在哪一列
    const QString &module_key = conf->get_module_key();
    const QList<QString> &module_field = conf->get_module_field();
    int module_column = module_field.indexOf( module_key );

    // 找出command_key在哪一列
    const QString &command_key = conf->get_command_key();
    const QList<QString> &command_field = conf->get_command_field();
    int command_column = command_field.indexOf( command_key );

    if ( module_column < 0 || command_column < 0 ) return;

    int select_row = -1;
    QTableWidget *select_tbl = NULL;
    const QList<QString> *select_field = NULL;
    for ( int idx = 0;idx < ui->module_tbl->rowCount();idx ++ )
    {
        const QTableWidgetItem *item = ui->module_tbl->item( idx,module_column );
        if ( item && item->text() == module )
        {
            select_row= idx;
            select_tbl = ui->module_tbl;
            select_field = &module_field;
            break;
        }
    }

    if ( select_row < 0 ) return;

    if (!command.isEmpty())
    {
        ui->module_tbl->setCurrentCell(
                select_row,module_column,QItemSelectionModel::ClearAndSelect );
        for ( int idx = 0;idx < ui->command_tbl->rowCount();idx ++ )
        {
            const QTableWidgetItem *item = ui->command_tbl->item( idx,command_column );
            if ( item && item->text() == command )
            {
                select_row = idx;
                select_tbl = ui->command_tbl;
                select_field = &command_field;
                break;
            }
        }
    }

    if ( select_row < 0 || !select_tbl || !select_field ) return;

    int select_column = select_field->indexOf( field );
    if ( select_column < 0 ) return;

    select_tbl->setCurrentCell( select_row,select_column,QItemSelectionModel::ClearAndSelect );
}

void MainWindow::save_proto()
{
    const QString &path = config::instance()->get_source_path();
    proto::instance()->save( path );
}
