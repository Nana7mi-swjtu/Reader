#include "reader.h"
#include "ui_reader.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QDateTime>
#include <QDebug>
#include <QMenu>
#include <QAction>
#include <QInputDialog>
#include <algorithm>
#include <QFile>
#include <QApplication>
#include <QScrollBar>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QShortcut>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QTimer>
#include <QVariantMap>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_sortMethod(0) // 默认按名称排序
    , m_currentTheme(0) // 默认浅色主题
    , m_currentFontSize(13) // 默认字体大小
    , zEpubParser(new readerform(this))//初始化epub解析器
    , zChapterDocument(nullptr)//初始化
    , zCurrentPage(1)//初始化章节页码
    , zTotalPage(1)//初始化总页码
    , zIsScorll(false)//初始化
{
    ui->setupUi(this);

    /*--------------------------------*/
    zChapterDocument = new QTextDocument(this);
    ui->readerTextBrowser->setDocument(zChapterDocument);//设置document实例，方便控制属性

    /*--------------------------------*/

    setupUI();
    loadSampleBooks();
    initWindowList();
    refreshBookLists();
    refreshCategoriesList();
    setupReaderNavigation();

    /*-----------------------------*/
    connect(ui->readerTextBrowser->verticalScrollBar(), &QScrollBar::valueChanged, this, &MainWindow::onReaderScroll);//连接 QTextBrowser 滚动条的 valueChanged 信号
    /*-----------------------------*/

    // 初始化主题 - 根据系统时间自动选择主题
    QTime currentTime = QTime::currentTime();
    if (currentTime.hour() >= 19 || currentTime.hour() < 7) {
        // 晚上7点到早上7点使用夜间模式
        m_currentTheme = 1;
        
        // 设置dark-theme属性
        this->setProperty("dark-theme", true);
        
        // 为列表小部件应用夜间模式类
        QList<QListWidget*> listWidgets = findChildren<QListWidget*>();
        for (QListWidget* widget : listWidgets) {
            widget->setProperty("dark-theme", true);
        }
        
        switchTheme(m_currentTheme);
        // 更新主题按钮图标
        QPushButton *themeButton = findChild<QPushButton*>("themeButton");
        if (themeButton) {
            themeButton->setIcon(QIcon(":/icons/dark.svg"));
        }
    } else {
        // 白天使用日间模式
        m_currentTheme = 0;
        
        // 设置dark-theme属性
        this->setProperty("dark-theme", false);
        
        // 为列表小部件应用日间模式类
        QList<QListWidget*> listWidgets = findChildren<QListWidget*>();
        for (QListWidget* widget : listWidgets) {
            widget->setProperty("dark-theme", false);
        }
        
        switchTheme(m_currentTheme);
        // 更新主题按钮图标
        QPushButton *themeButton = findChild<QPushButton*>("themeButton");
        if (themeButton) {
            themeButton->setIcon(QIcon(":/icons/light.svg"));
        }
    }
    ui->readerTextBrowser->installEventFilter(this);
    QShortcut *zoomInShortcut = new QShortcut(QKeySequence("Ctrl++"), this);
    QShortcut *zoomOutShortcut = new QShortcut(QKeySequence("Ctrl+-"), this);

    // 添加书签快捷键
    QShortcut *bookmarkShortcut = new QShortcut(QKeySequence("Ctrl+M"), this);
    connect(bookmarkShortcut, &QShortcut::activated, this, &MainWindow::on_bookmarkButton_clicked);


}
bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (ui->contentStackedWidget->currentWidget() == ui->readerPage &&
        watched == ui->readerTextBrowser)
    {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->key() == Qt::Key_Left) {
                gotoPreviousPage();
                return true;
            } else if (keyEvent->key() == Qt::Key_Right) {
                gotoNextPage();
                return true;
            }
        }
        else if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                gotoPreviousPage();
                return true;
            } else if (mouseEvent->button() == Qt::RightButton) {
                gotoNextPage();
                return true;
            }
        }
    }

    return QMainWindow::eventFilter(watched, event);
}
MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupUI()
{
    // 设置窗口标题
    setWindowTitle(tr("myReader - 电子书阅读器"));
    
    // 连接搜索框的信号
    connect(ui->searchLineEdit, &QLineEdit::textChanged, this, &MainWindow::on_searchLineEdit_textChanged);
    connect(ui->favSearchLineEdit, &QLineEdit::textChanged, this, &MainWindow::on_favSearchLineEdit_textChanged);
    connect(ui->categorySearchLineEdit, &QLineEdit::textChanged, this, &MainWindow::on_categorySearchLineEdit_textChanged);
    
    // 自定义书籍列表项样式
    ui->allBooksListWidget->setIconSize(QSize(48, 48));
    ui->favPageListWidget->setIconSize(QSize(48, 48));
    ui->categoryListWidget->setIconSize(QSize(48, 48));
    
    // 设置排序按钮图标
    ui->sortByNameButton->setIcon(QIcon(":/icons/name.svg"));
    ui->sortByTimeButton->setIcon(QIcon(":/icons/time.svg"));
    ui->sortByRecentButton->setIcon(QIcon(":/icons/recent.svg"));
    
    // 设置其他按钮图标
    ui->searchButton->setIcon(QIcon(":/icons/search.svg"));
    ui->favSearchButton->setIcon(QIcon(":/icons/search.svg"));
    ui->categorySearchButton->setIcon(QIcon(":/icons/search.svg"));
    ui->addCategoryButton->setIcon(QIcon(":/icons/add.svg"));
    ui->bookmarkButton->setIcon(QIcon(":/icons/bookmark.svg"));
    
    // 设置初始排序按钮状态
    ui->sortByNameButton->setChecked(true);
    
    // 设置窗口列表右键菜单
    ui->windowListWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->windowListWidget, &QWidget::customContextMenuRequested, 
            this, &MainWindow::onWindowListContextMenu);
            
    // 设置分类列表右键菜单
    ui->categoriesListWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->categoriesListWidget, &QWidget::customContextMenuRequested,
            this, &MainWindow::onCategoriesListContextMenu);
    
    // 添加主题切换按钮
    QPushButton *themeButton = new QPushButton(this);
    themeButton->setObjectName("themeButton");
    themeButton->setIcon(QIcon(":/icons/light.svg"));
    themeButton->setFlat(true);
    themeButton->setToolTip(tr("切换主题"));
    ui->horizontalLayout->addWidget(themeButton);
    connect(themeButton, &QPushButton::clicked, this, &MainWindow::on_themeButton_clicked);

}

void MainWindow::initWindowList()
{
    // 清空窗口列表
    ui->windowListWidget->clear();
    m_windows.clear();
    
    // 添加主窗口和收藏夹窗口
    addWindow(MAIN_WINDOW, tr("主界面"));
    addWindow(FAVORITES, tr("收藏夹"));
    
    // 默认选择主窗口
    ui->windowListWidget->item(0)->setSelected(true);
    switchToWindow(0);
}

void MainWindow::addWindow(WindowType type, const QString &title, const QString &identifier)
{
    // 创建窗口信息
    WindowInfo info(type, title, identifier);
    m_windows.append(info);
    
    // 创建列表项
    QListWidgetItem *item = new QListWidgetItem(ui->windowListWidget);
    item->setText(title);
    item->setData(Qt::UserRole, m_windows.size() - 1); // 存储窗口索引
    
    // 设置图标
    switch(type) {
        case MAIN_WINDOW:
            item->setIcon(QIcon(":/icons/home.svg"));
            break;
        case FAVORITES:
            item->setIcon(QIcon(":/icons/favorite.svg"));
            break;
        case CATEGORY:
            item->setIcon(QIcon(":/icons/folder.svg"));
            break;
        case BOOK_READER:
            item->setIcon(QIcon(":/icons/book.svg"));
            break;
    }
}

void MainWindow::removeWindow(int index)
{
    if (index < 0 || index >= m_windows.size() || index < 2) {
        // 不能删除主窗口和收藏夹
        return;
    }
    
    // 删除窗口列表项
    delete ui->windowListWidget->takeItem(index);
    
    // 移除窗口信息
    m_windows.removeAt(index);
    
    // 更新其他窗口项的索引
    for (int i = 0; i < ui->windowListWidget->count(); ++i) {
        QListWidgetItem *item = ui->windowListWidget->item(i);
        int oldIndex = item->data(Qt::UserRole).toInt();
        if (oldIndex > index) {
            item->setData(Qt::UserRole, oldIndex - 1);
        }
    }
    
    // 切换到主窗口
    ui->windowListWidget->item(0)->setSelected(true);
    switchToWindow(0);
}

void MainWindow::switchToWindow(int index)
{
    if (index < 0 || index >= m_windows.size()) {
        return;
    }
    
    const WindowInfo &info = m_windows[index];
    
    // 根据窗口类型切换到相应页面
    switch(info.type) {
        case MAIN_WINDOW:
            ui->contentStackedWidget->setCurrentWidget(ui->mainPage);
            break;
        case FAVORITES:
            updateFavoritesPage();
            ui->contentStackedWidget->setCurrentWidget(ui->favoritesPage);
            break;
        case CATEGORY:
            updateCategoryPage(info.identifier);
            ui->contentStackedWidget->setCurrentWidget(ui->categoryPage);
            ui->categoryTitleLabel->setText(info.title);
            break;
        case BOOK_READER:
            ui->bookTitleLabel->setText(info.title);
            ui->contentStackedWidget->setCurrentWidget(ui->readerPage);
            
            // 确保应用当前主题样式
            QString bgColor = m_currentTheme == 1 ? "#121212" : "#FEF9E7";
            QString textColor = m_currentTheme == 1 ? "#C8C8C8" : "#1C2833";
            
            QString style = QString(
                "QTextBrowser#readerTextBrowser {"
                "    background-color: %1 !important;"
                "    color: %2 !important;"
                "    border: none;"
                "    line-height: 1.8;"
                "    text-align: justify;"
                "    letter-spacing: 0.2px;"
                "    padding: 24px 32px;"
                "    border-radius: 8px;"
                "}"
            ).arg(bgColor, textColor);
            
            ui->readerTextBrowser->setStyleSheet(style);
            break;
    }
}

void MainWindow::updateFavoritesPage()
{
    // 更新收藏夹页面内容
    ui->favPageListWidget->clear();
    
    QList<QString> favorites = getFavoriteBooks();
    for (const QString &path : favorites) {
        if (allBooks.contains(path)) {
            addBookToList(ui->favPageListWidget, allBooks[path]);
        }
    }
}

void MainWindow::updateCategoryPage(const QString &categoryName)
{
    // 更新分类页面内容
    ui->categoryListWidget->clear();
    
    if (!m_categories.contains(categoryName)) {
        return;
    }
    
    QList<QString> books = m_categories[categoryName].books;
    for (const QString &path : books) {
        if (allBooks.contains(path)) {
            addBookToList(ui->categoryListWidget, allBooks[path]);
        }
    }
}

void MainWindow::refreshBookLists()
{
    // 清空现有列表
    ui->allBooksListWidget->clear();
    
    // 提取所有书籍并进行排序
    QList<BookInfo> sortedBooks = allBooks.values();
    
    // 根据排序方法排序
    if (m_sortMethod == 0) {
        // 按名称排序
        std::sort(sortedBooks.begin(), sortedBooks.end(), [](const BookInfo &a, const BookInfo &b) {
            return a.title < b.title;
        });
    } else if (m_sortMethod == 1) {
        // 按阅读时间排序
        std::sort(sortedBooks.begin(), sortedBooks.end(), [](const BookInfo &a, const BookInfo &b) {
            return a.totalReadTime > b.totalReadTime;
        });
    } else if (m_sortMethod == 2) {
        // 按最近阅读排序
        std::sort(sortedBooks.begin(), sortedBooks.end(), [](const BookInfo &a, const BookInfo &b) {
            return a.lastReadTime > b.lastReadTime;
        });
    }
    
    // 添加所有书籍到全部书籍列表
    for (const auto &book : sortedBooks) {
        addBookToList(ui->allBooksListWidget, book);
    }
}

void MainWindow::addBookToList(QListWidget *listWidget, const BookInfo &book)
{
    QListWidgetItem *item = new QListWidgetItem(listWidget);
    
    // 计算时间显示格式
    QString timeStr;
    int hours = book.totalReadTime.hour();
    int minutes = book.totalReadTime.minute();
    
    if (hours > 0) {
        timeStr = QString("%1时%2分").arg(hours).arg(minutes);
    } else {
        timeStr = QString("%1分").arg(minutes);
    }
    
    // 设置显示文本
    QString displayText = QString("%1\n阅读时间: %2").arg(book.title, timeStr);
    
    item->setText(displayText);
    item->setData(Qt::UserRole, book.filePath); // 在用户数据中存储文件路径
    
    // 添加默认图标
    item->setIcon(QIcon(":/icons/book.svg"));
}

void MainWindow::sortBooks(int sortMethod)
{
    m_sortMethod = sortMethod;
    refreshBookLists();
}

void MainWindow::loadSampleBooks()
{
    // 添加一些示例书籍数据
    BookInfo book1;
    book1.filePath = "sample/book1.epub";
    book1.title = "三体";
    book1.totalReadTime = QTime(5, 30); // 5小时30分钟
    book1.isFavorite = true;
    book1.lastReadTime = QDateTime::currentDateTime().addDays(-1);
    
    BookInfo book2;
    book2.filePath = "sample/book2.epub";
    book2.title = "活着";
    book2.totalReadTime = QTime(2, 45);
    book2.isFavorite = false;
    book2.lastReadTime = QDateTime::currentDateTime().addDays(-5);
    
    BookInfo book3;
    book3.filePath = "sample/book3.epub";
    book3.title = "百年孤独";
    book3.totalReadTime = QTime(8, 15);
    book3.isFavorite = true;
    book3.lastReadTime = QDateTime::currentDateTime().addDays(-2);
    
    BookInfo book4;
    book4.filePath = "sample/book4.epub";
    book4.title = "围城";
    book4.totalReadTime = QTime(3, 20);
    book4.isFavorite = false;
    book4.lastReadTime = QDateTime::currentDateTime().addDays(-10);
    
    BookInfo book5;
    book5.filePath = "sample/book5.epub";
    book5.title = "平凡的世界";
    book5.totalReadTime = QTime(12, 10);
    book5.isFavorite = true;
    book5.lastReadTime = QDateTime::currentDateTime();
    
    // 添加到书籍集合
    allBooks[book1.filePath] = book1;
    allBooks[book2.filePath] = book2;
    allBooks[book3.filePath] = book3;
    allBooks[book4.filePath] = book4;
    allBooks[book5.filePath] = book5;
    
    // 创建示例分类
    createCategory("文学");
    createCategory("科幻");
    createCategory("哲学");
    
    // 将书籍添加到分类
    addBookToCategory("sample/book1.epub", "科幻");
    addBookToCategory("sample/book2.epub", "文学");
    addBookToCategory("sample/book3.epub", "文学");
    addBookToCategory("sample/book4.epub", "哲学");
    addBookToCategory("sample/book5.epub", "文学");
}

void MainWindow::createCategory(const QString &name)
{
    if (name.isEmpty() || m_categories.contains(name)) {
        return;
    }
    
    CategoryInfo category(name);
    m_categories[name] = category;
}

void MainWindow::addBookToCategory(const QString &filePath, const QString &categoryName)
{
    if (!allBooks.contains(filePath) || !m_categories.contains(categoryName)) {
        return;
    }
    
    // 将书籍添加到分类
    m_categories[categoryName].books.append(filePath);
    
    // 在书籍中添加分类信息
    if (!allBooks[filePath].categories.contains(categoryName)) {
        allBooks[filePath].categories.append(categoryName);
    }
}

void MainWindow::refreshCategoriesList()
{
    ui->categoriesListWidget->clear();
    
    for (auto it = m_categories.begin(); it != m_categories.end(); ++it) {
        QListWidgetItem *item = new QListWidgetItem(ui->categoriesListWidget);
        item->setText(it.key());
        item->setIcon(QIcon(":/icons/folder.svg"));
        item->setData(Qt::UserRole, it.key()); // 存储分类名称
    }
}

void MainWindow::toggleSearchVisibility(QLineEdit *searchEdit, bool visible)
{
    searchEdit->setVisible(visible);
    if (visible) {
        searchEdit->setFocus();
        searchEdit->clear();
    }
}

QList<QString> MainWindow::getBooksInCategory(const QString &categoryName)
{
    if (m_categories.contains(categoryName)) {
        return m_categories[categoryName].books;
    }
    return QList<QString>();
}

QList<QString> MainWindow::getFavoriteBooks()
{
    QList<QString> favorites;
    for (auto it = allBooks.begin(); it != allBooks.end(); ++it) {
        if (it.value().isFavorite) {
            favorites.append(it.key());
        }
    }
    return favorites;
}

void MainWindow::searchBooks(QListWidget *listWidget, const QString &text, const QList<QString> &bookPaths)
{
    listWidget->clear();
    
    if (text.isEmpty()) {
        // 如果搜索文本为空，显示所有书籍
        for (const QString &path : bookPaths) {
            if (allBooks.contains(path)) {
                addBookToList(listWidget, allBooks[path]);
            }
        }
    } else {
        // 否则只显示匹配的书籍
        for (const QString &path : bookPaths) {
            if (allBooks.contains(path) && allBooks[path].title.contains(text, Qt::CaseInsensitive)) {
                addBookToList(listWidget, allBooks[path]);
            }
        }
    }
}

void MainWindow::on_windowListWidget_itemClicked(QListWidgetItem *item)
{
    if (item) {
        int index = item->data(Qt::UserRole).toInt();
        switchToWindow(index);
    }
}

void MainWindow::on_addBookButton_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("打开电子书"),
                                                  "", tr("电子书 (*.epub)"));
    if (!filePath.isEmpty()) {
        if (!allBooks.contains(filePath)) {
            BookInfo newBook;
            newBook.filePath = filePath;
            QFileInfo fileInfo(filePath);
            newBook.title = fileInfo.baseName();
            newBook.totalReadTime = QTime(0, 0);
            newBook.isFavorite = false;
            newBook.lastReadTime = QDateTime::currentDateTime();
            
            allBooks[filePath] = newBook;
            refreshBookLists();
        }
        
        openBook(filePath);
    }
}

void MainWindow::on_sortByNameButton_clicked()
{
    sortBooks(0);
}

void MainWindow::on_sortByTimeButton_clicked()
{
    sortBooks(1);
}

void MainWindow::on_sortByRecentButton_clicked()
{
    sortBooks(2);
}

void MainWindow::on_closeBookButton_clicked()
{
    // 关闭书籍时保存书签
    if (!zCurrentBookFikePath.isEmpty())
    {
        saveBookmarkInfo(zCurrentBookFikePath); // 调用保存函数
    }
    // 获取当前窗口索引
    QList<QListWidgetItem*> selectedItems = ui->windowListWidget->selectedItems();
    if (!selectedItems.isEmpty()) {
        int index = selectedItems.first()->data(Qt::UserRole).toInt();
        if (m_windows[index].type == BOOK_READER) {
            removeWindow(index);
        }
    }
}

void MainWindow::on_bookmarkButton_clicked()
{
    QString currentFilePath = zCurrentBookFikePath;
    if (currentFilePath.isEmpty()) return;

    auto it = allBooks.find(currentFilePath);
    /*if (it != allBooks.end()) {
        it->BookMarks.insert(zCurrentPage, QString("第 %1 页").arg(zCurrentPage));
        updateBookmarkComboBox();
        QMessageBox::information(this, "添加书签", QString("已在第 %1 页添加书签").arg(zCurrentPage));
    }*/
    QString chapterTitle;//章节标题
    QMap<QString, QString> toc = zEpubParser->getTableofContent();
    if (toc.contains(zCurrentChapterId))
    {
        chapterTitle = toc.value(zCurrentChapterId);
    }
    bookMark newMark;
    newMark.chapterId = zCurrentChapterId;
    newMark.chapterTitle = chapterTitle;
    newMark.pageInChapter = zCurrentPage;
    if (it != allBooks.end())
    {
        it->BookMarks.append(newMark);
        updateBookmarkComboBox();
        QMessageBox::information(this, "添加书签", QString("已在%1 第 %2 页添加书签").arg(chapterTitle).arg(zCurrentPage));
    }
    

}

void MainWindow::on_addToButton_clicked()
{
    // 获取当前窗口索引
    QList<QListWidgetItem*> selectedItems = ui->windowListWidget->selectedItems();
    if (!selectedItems.isEmpty()) {
        int index = selectedItems.first()->data(Qt::UserRole).toInt();
        if (m_windows[index].type != BOOK_READER) {
            return;
        }
        
        QString filePath = m_windows[index].identifier;
        if (!allBooks.contains(filePath)) {
            return;
        }
        
        // 创建添加到菜单
        QMenu addToMenu(this);
        
        // 添加到收藏夹
        QAction *addToFavAction = addToMenu.addAction(tr("添加到收藏夹"));
        
        // 添加到现有分类
        QMenu *categoriesMenu = addToMenu.addMenu(tr("添加到分类"));
        for (auto it = m_categories.begin(); it != m_categories.end(); ++it) {
            QAction *categoryAction = categoriesMenu->addAction(it.key());
            categoryAction->setData(it.key());
        }
        
        // 创建新分类
        QAction *newCategoryAction = addToMenu.addAction(tr("创建新分类..."));
        
        // 显示菜单并处理选择
        QAction *selectedAction = addToMenu.exec(QCursor::pos());
        
        if (selectedAction == addToFavAction) {
            // 添加到收藏夹
            allBooks[filePath].isFavorite = true;
            QMessageBox::information(this, tr("添加到收藏夹"), 
                                   tr("《%1》已添加到收藏夹").arg(allBooks[filePath].title));
            
            // 刷新收藏夹（如果打开的话）
            for (int i = 0; i < m_windows.size(); ++i) {
                if (m_windows[i].type == FAVORITES && 
                    ui->contentStackedWidget->currentWidget() == ui->favoritesPage) {
                    updateFavoritesPage();
                    break;
                }
            }
        } else if (selectedAction == newCategoryAction) {
            // 创建新分类
            QString categoryName = QInputDialog::getText(this, tr("创建新分类"), 
                                                      tr("请输入分类名称:"));
            if (!categoryName.isEmpty()) {
                createCategory(categoryName);
                addBookToCategory(filePath, categoryName);
                refreshCategoriesList();
                QMessageBox::information(this, tr("添加到分类"), 
                                       tr("《%1》已添加到分类 %2").arg(allBooks[filePath].title, categoryName));
            }
        } else if (selectedAction && categoriesMenu->actions().contains(selectedAction)) {
            // 添加到现有分类
            QString categoryName = selectedAction->data().toString();
            addBookToCategory(filePath, categoryName);
            QMessageBox::information(this, tr("添加到分类"), 
                                   tr("《%1》已添加到分类 %2").arg(allBooks[filePath].title, categoryName));
            
            // 刷新分类页面（如果打开的话）
            for (int i = 0; i < m_windows.size(); ++i) {
                if (m_windows[i].type == CATEGORY && 
                    m_windows[i].identifier == categoryName &&
                    ui->contentStackedWidget->currentWidget() == ui->categoryPage) {
                    updateCategoryPage(categoryName);
                    break;
                }
            }
        }
    }
}

void MainWindow::on_addCategoryButton_clicked()
{
    QString categoryName = QInputDialog::getText(this, tr("创建新分类"), 
                                              tr("请输入分类名称:"));
    if (!categoryName.isEmpty()) {
        createCategory(categoryName);
        refreshCategoriesList();
    }
}

void MainWindow::on_searchButton_clicked()
{
    toggleSearchVisibility(ui->searchLineEdit, !ui->searchLineEdit->isVisible());
}

void MainWindow::on_favSearchButton_clicked()
{
    toggleSearchVisibility(ui->favSearchLineEdit, !ui->favSearchLineEdit->isVisible());
}

void MainWindow::on_categorySearchButton_clicked()
{
    toggleSearchVisibility(ui->categorySearchLineEdit, !ui->categorySearchLineEdit->isVisible());
}

void MainWindow::onWindowListContextMenu(const QPoint &pos)
{
    QListWidgetItem *item = ui->windowListWidget->itemAt(pos);
    if (!item) {
        return;
    }
    
    int index = item->data(Qt::UserRole).toInt();
    if (index < 2) {
        // 主界面和收藏夹不能关闭
        return;
    }
    
    QMenu contextMenu(this);
    QAction *closeAction = contextMenu.addAction(tr("关闭"));
    
    QAction *selectedAction = contextMenu.exec(ui->windowListWidget->mapToGlobal(pos));
    if (selectedAction == closeAction) {
        removeWindow(index);
    }
}

void MainWindow::on_searchLineEdit_textChanged(const QString &text)
{
    if (text.isEmpty()) {
        refreshBookLists();
        return;
    }
    
    ui->allBooksListWidget->clear();
    
    // 提取所有书籍并进行排序
    QList<BookInfo> sortedBooks = allBooks.values();
    
    // 根据排序方法排序
    if (m_sortMethod == 0) {
        // 按名称排序
        std::sort(sortedBooks.begin(), sortedBooks.end(), [](const BookInfo &a, const BookInfo &b) {
            return a.title < b.title;
        });
    } else if (m_sortMethod == 1) {
        // 按阅读时间排序
        std::sort(sortedBooks.begin(), sortedBooks.end(), [](const BookInfo &a, const BookInfo &b) {
            return a.totalReadTime > b.totalReadTime;
        });
    } else if (m_sortMethod == 2) {
        // 按最近阅读排序
        std::sort(sortedBooks.begin(), sortedBooks.end(), [](const BookInfo &a, const BookInfo &b) {
            return a.lastReadTime > b.lastReadTime;
        });
    }
    
    // 添加匹配的书籍到列表
    for (const auto &book : sortedBooks) {
        if (book.title.contains(text, Qt::CaseInsensitive)) {
            addBookToList(ui->allBooksListWidget, book);
        }
    }
}

void MainWindow::on_favSearchLineEdit_textChanged(const QString &text)
{
    QList<QString> favorites = getFavoriteBooks();
    searchBooks(ui->favPageListWidget, text, favorites);
}

void MainWindow::on_categorySearchLineEdit_textChanged(const QString &text)
{
    int index = -1;
    for (int i = 0; i < m_windows.size(); ++i) {
        if (m_windows[i].type == CATEGORY && 
            ui->contentStackedWidget->currentWidget() == ui->categoryPage &&
            ui->categoryTitleLabel->text() == m_windows[i].title) {
            index = i;
            break;
        }
    }
    
    if (index == -1) {
        return;
    }

    QString categoryName = m_windows[index].identifier;
    QList<QString> categoryBooks = getBooksInCategory(categoryName);
    searchBooks(ui->categoryListWidget, text, categoryBooks);
}

void MainWindow::onCategoriesListContextMenu(const QPoint &pos)
{
    QListWidgetItem *item = ui->categoriesListWidget->itemAt(pos);
    if (!item) {
        return;
    }
    
    QString categoryName = item->data(Qt::UserRole).toString();
    
    QMenu contextMenu(this);
    QAction *openAction = contextMenu.addAction(tr("打开"));
    QAction *renameAction = contextMenu.addAction(tr("重命名"));
    QAction *deleteAction = contextMenu.addAction(tr("删除"));
    
    QAction *selectedAction = contextMenu.exec(ui->categoriesListWidget->mapToGlobal(pos));
    
    if (selectedAction == openAction) {
        on_categoriesListWidget_itemDoubleClicked(item);
    } else if (selectedAction == renameAction) {
        QString newName = QInputDialog::getText(this, tr("重命名分类"), 
                                             tr("请输入新名称:"), QLineEdit::Normal, categoryName);
        if (!newName.isEmpty() && newName != categoryName && !m_categories.contains(newName)) {
            CategoryInfo newCategory = m_categories[categoryName];
            newCategory.name = newName;
            m_categories[newName] = newCategory;

            for (const QString &path : newCategory.books) {
                if (allBooks.contains(path)) {
                    QList<QString> &categories = allBooks[path].categories;
                    int index = categories.indexOf(categoryName);
                    if (index != -1) {
                        categories.replace(index, newName);
                    }
                }
            }
            
            // 删除旧分类
            m_categories.remove(categoryName);
            
            // 更新窗口信息
            for (int i = 0; i < m_windows.size(); ++i) {
                if (m_windows[i].type == CATEGORY && m_windows[i].identifier == categoryName) {
                    m_windows[i].title = newName;
                    m_windows[i].identifier = newName;
                    ui->windowListWidget->item(i)->setText(newName);
                    
                    if (ui->contentStackedWidget->currentWidget() == ui->categoryPage && 
                        ui->categoryTitleLabel->text() == categoryName) {
                        ui->categoryTitleLabel->setText(newName);
                    }
                    break;
                }
            }

            refreshCategoriesList();
        }
    } else if (selectedAction == deleteAction) {
        QMessageBox::StandardButton reply = QMessageBox::question(this, tr("删除分类"), 
                                                             tr("确定要删除分类 %1 吗？").arg(categoryName),
                                                             QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            for (const QString &path : m_categories[categoryName].books) {
                if (allBooks.contains(path)) {
                    allBooks[path].categories.removeAll(categoryName);
                }
            }

            m_categories.remove(categoryName);

            for (int i = 0; i < m_windows.size(); ++i) {
                if (m_windows[i].type == CATEGORY && m_windows[i].identifier == categoryName) {
                    removeWindow(i);
                    break;
                }
            }

            refreshCategoriesList();
        }
    }
}

void MainWindow::on_allBooksListWidget_itemDoubleClicked(QListWidgetItem *item)
{
    if (item) {
        QString filePath = item->data(Qt::UserRole).toString();
        openBook(filePath);
    }
}

void MainWindow::on_categoriesListWidget_itemDoubleClicked(QListWidgetItem *item)
{
    if (item) {
        QString categoryName = item->data(Qt::UserRole).toString();

        // 检查是否已经打开了了这个分类
        for (int i = 0; i < m_windows.size(); ++i) {
            if (m_windows[i].type == CATEGORY && m_windows[i].identifier == categoryName) {
                ui->windowListWidget->item(i)->setSelected(true);
                switchToWindow(i);
                return;
            }
        }

        addWindow(CATEGORY, categoryName, categoryName);

        ui->windowListWidget->item(ui->windowListWidget->count() - 1)->setSelected(true);
        switchToWindow(m_windows.size() - 1);
    }
}

void MainWindow::on_favPageListWidget_itemDoubleClicked(QListWidgetItem *item)
{
    if (item) {
        QString filePath = item->data(Qt::UserRole).toString();
        openBook(filePath);
    }
}

void MainWindow::on_categoryListWidget_itemDoubleClicked(QListWidgetItem *item)
{
    if (item) {
        QString filePath = item->data(Qt::UserRole).toString();
        openBook(filePath);
    }
}

void MainWindow::openBook(const QString &filePath)
{
    if (!allBooks.contains(filePath)) {
        return;
    }
    
    
    /*-----------------------------------------------------*/

    zEpubParser->closeEpub();//先关闭

    if (!zEpubParser->openEpub(filePath))
    {
        QMessageBox::critical(this, tr("failure occur when open epub file"), tr("could open epub file%1,error:%2").arg(filePath).arg(zEpubParser->getLastError()));

        zCurrentBookFikePath.clear();//清空路径
        if (zChapterDocument)
        {
            zChapterDocument->setHtml("");//内容设置为空
        }
        zCurrentChapterId.clear();//id清空
        updatePagination();//重置分页
        goToPage(1);
        return;
    }

    zCurrentBookFikePath = filePath;
    QVariantMap metadata = zEpubParser->getMetaDate();//获取元数据
    QString epubTitle = metadata.value("title", QFileInfo(filePath).baseName()).toString();//获取标题

    if (epubTitle.isEmpty())//如果标题为空
    {
        epubTitle = QFileInfo(filePath).baseName();//使用文件名作为标题
    }

    if (allBooks.contains(filePath))//更新标题
    {
        allBooks[filePath].title = epubTitle;
    }

    QList<SpineItem> spine = zEpubParser->getSpineItem();

    /*----------------------*/
    zCurrentBookSpineId.clear();//清空
    zCurrentBookItemIndex = -1;

    for (const SpineItem& spineitem : spine)//依次插入
    {
        if (spineitem.linear)
        {
            zCurrentBookSpineId.append(spineitem.idref);
        }
    }
    /*----------------------*/

    QString firstId;
    if (!zCurrentBookSpineId.isEmpty())
    {
        firstId = zCurrentBookSpineId.first();
    }

    /*-----------------------------------------------------*/

    BookInfo& book = allBooks[filePath];
    

    // 检查是否已经打开了这本书
    for (int i = 0; i < m_windows.size(); ++i) {
        if (m_windows[i].type == BOOK_READER && m_windows[i].identifier == filePath) {
            // 已经打开，直接切换到此窗口
            ui->windowListWidget->item(i)->setSelected(true);
            switchToWindow(i);
            return;
        }
    }

    addWindow(BOOK_READER, book.title, filePath);
    ui->windowListWidget->item(ui->windowListWidget->count() - 1)->setSelected(true);
    switchToWindow(m_windows.size() - 1);

    book.lastReadTime = QDateTime::currentDateTime();

    /*ui->pageSlider->setValue(1);*/

    /*QFont font = ui->readerTextBrowser->font();
    font.setPointSize(m_currentFontSize);
    ui->readerTextBrowser->setFont(font);*/

    QString bgColor = m_currentTheme == 1 ? "#121212" : "#FEF9E7";
    QString textColor = m_currentTheme == 1 ? "#C8C8C8" : "#1C2833";
    QString style = QString(
        "QTextBrowser#readerTextBrowser {"
        "    background-color: %1 !important;"
        "    color: %2 !important;"
        "    border: none;"
        "    line-height: 1.8;"
        "    text-align: justify;"
        "    letter-spacing: 0.2px;"
        "    padding: 24px 32px;"
        "    border-radius: 8px;"
        "}"
    ).arg(bgColor, textColor);
    
    ui->readerTextBrowser->setStyleSheet(style);
    
    /*-----------------------------*/
    if (zChapterDocument)
    {
        QFont currentFont = ui->readerTextBrowser->font();
        currentFont.setPointSize(m_currentFontSize);
        zChapterDocument->setDefaultFont(currentFont);
    }


    /*-----------------------------*/
    // 应用淡入效果
    QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect(ui->readerTextBrowser);
    ui->readerTextBrowser->setGraphicsEffect(effect);
    
    QPropertyAnimation *animation = new QPropertyAnimation(effect, "opacity");
    animation->setDuration(500);
    animation->setStartValue(0.0);
    animation->setEndValue(1.0);
    animation->setEasingCurve(QEasingCurve::InOutQuad);
    
    // 设置内容
    /*QString sampleContent;
    ui->readerTextBrowser->setText(sampleContent);*/
    
    // 启动动画
    animation->start(QAbstractAnimation::DeleteWhenStopped);
    
    // 滚动到顶部
    ui->readerTextBrowser->verticalScrollBar()->setValue(0);

    /*---------------------------*/
    if (!firstId.isEmpty())
    {
        QTimer::singleShot(0, this, [this,firstId]() {
            QFont currentFont = zChapterDocument->defaultFont();
            if (currentFont.pointSize() != m_currentFontSize)
            {
                currentFont.setPointSize(m_currentFontSize);
                zChapterDocument->setDefaultFont(currentFont);
                ui->readerTextBrowser->setFont(currentFont);
            }
            loadChapter(firstId);
            });
    }

    else
    {
        if (zChapterDocument)
        {
            zChapterDocument->setHtml("<p>error</p>");
        }
        QMessageBox::critical(this, tr("load error"), tr("no chapter"));
        return;
    }


    /*---------------------------*/

    
    // 更新状态栏信息
    ui->statusbar->showMessage(tr("当前阅读：《%1》").arg(book.title), 3000);
}

void MainWindow::on_themeButton_clicked()
{
    // 切换主题
    m_currentTheme = (m_currentTheme == 0) ? 1 : 0;
    switchTheme(m_currentTheme);
    
    // 更新按钮图标
    QPushButton *themeButton = findChild<QPushButton*>("themeButton");
    if (themeButton) {
        themeButton->setIcon(QIcon(m_currentTheme == 0 ? ":/icons/light.svg" : ":/icons/dark.svg"));
    }
    
    // 直接强制更新当前阅读界面的样式（如果有打开的书籍）
    if (ui->contentStackedWidget->currentWidget() == ui->readerPage) {
        // 设置新的背景和文本颜色
        QString bgColor = m_currentTheme == 1 ? "#121212" : "#FEF9E7";
        QString textColor = m_currentTheme == 1 ? "#C8C8C8" : "#1C2833";
        
        // 创建样式表字符串
        QString style = QString(
            "QTextBrowser#readerTextBrowser {"
            "    background-color: %1 !important;"
            "    color: %2 !important;"
            "    border: none;"
            "    line-height: 1.8;"
            "    text-align: justify;"
            "    letter-spacing: 0.2px;"
            "    padding: 24px 32px;"
            "    border-radius: 8px;"
            "}"
        ).arg(bgColor, textColor);
        
        // 应用样式
        ui->readerTextBrowser->setStyleSheet(style);
    }
}

void MainWindow::switchTheme(int themeIndex)
{
    // 读取样式表
    QFile styleFile(":/app_style.qss");
    if (!styleFile.open(QFile::ReadOnly)) {
        return;
    }
    
    QString styleSheet = QLatin1String(styleFile.readAll());
    styleFile.close();
    
    if (themeIndex == 1) {
        // 深色主题
        ui->bookmarkComboBox->setStyleSheet(
            "QComboBox {"
            "    background-color: #252525;"
            "    color: #C8C8C8;"
            "    border: 1px solid #2C2C2C;"
            "    border-radius: 4px;"
            "    padding: 4px 8px;"
            "    min-height: 24px;"
            "}"
            "QComboBox:hover {"
            "    background-color: #2A2A2A;"
            "    border: 1px solid #3D6889;"
            "}"
            "QComboBox::drop-down {"
            "    border: none;"
            "    width: 20px;"
            "}"
            "QComboBox::down-arrow {"
            "    image: url(:/icons/down_arrow_white.svg);"
            "    width: 12px;"
            "    height: 12px;"
            "}"
            "QComboBox QAbstractItemView {"
            "    background-color: #252525;"
            "    color: #C8C8C8;"
            "    border: 1px solid #2C2C2C;"
            "    selection-background-color: #3D6889;"
            "    selection-color: #FFFFFF;"
            "}"
            );
        styleSheet.replace("#F9F9F9", "#121212"); // 背景色
        styleSheet.replace("#FFFFFF", "#1A1A1A"); // 控件背景
        styleSheet.replace("#DDDDDD", "#2C2C2C"); // 边框色
        styleSheet.replace("#F2F2F2", "#1C1C1C"); // 次要背景
        styleSheet.replace("#F0F0F0", "#252525"); // 按钮悬停背景色
        styleSheet.replace("#E5E5E5", "#1E1E1E"); // 按钮按下背景色
        styleSheet.replace("#333333", "#B0B0B0"); // 主文本色
        styleSheet.replace("#444444", "#A0A0A0"); // 次要文本色
        styleSheet.replace("#666666", "#909090"); // 状态栏文本色
        styleSheet.replace("#2196F3", "#3D6889"); // 主题色 - 降低亮度
        styleSheet.replace("#1976D2", "#2C5170"); // 次要主题色
        styleSheet.replace("#E3F2FD", "#141E26"); // 选中背景色
        styleSheet.replace("#F5F5F5", "#1E1E1E"); // 悬浮背景色
        styleSheet.replace("#EEEEEE", "#282828"); // 分隔线颜色
        styleSheet.replace("#2C3E50", "#CCCCCC"); // 阅读器文本颜色
        styleSheet.replace("#FFFDF7", "#121212"); // 阅读器背景颜色
        styleSheet.replace("#FEF9E7", "#121212"); // 新的阅读器背景颜色
        styleSheet.replace("#1C2833", "#C0C0C0"); // 新的阅读器文本颜色
        
        // 分类相关样式
        styleSheet.replace("#F0F7FF", "#252525"); // 分类悬停背景
        styleSheet.replace("#E8F5E9", "#252525"); // 分类悬停背景(绿色)
        styleSheet.replace("#F1F8E9", "#252525"); // 书籍列表悬停
        styleSheet.replace("#DCEDC8", "#252525"); // 分类选中背景
        
        // 修改选中项的背景颜色
        styleSheet.append("\n#allBooksListWidget::item:selected, #favPageListWidget::item:selected, "
                          "#categoryListWidget::item:selected { background-color: #252525 !important; color: #E0E0E0 !important; }\n");
        
        // 为列表项添加特殊的夜间模式样式
        styleSheet.append("\n#allBooksListWidget::item, #favPageListWidget::item, "
                          "#categoryListWidget::item { background-color: #1D1D1D; color: #B0B0B0; border-bottom: 1px solid #282828; }\n");
        
        styleSheet.append("\n#allBooksListWidget::item:hover, #favPageListWidget::item:hover, "
                          "#categoryListWidget::item:hover { background-color: #2A2A2A; }\n");
        
        // 绿色元素的暗色替换 - 减少饱和度
        styleSheet.replace("#4CAF50", "#45705E");
        styleSheet.replace("#43A047", "#3E6455");
        styleSheet.replace("#388E3C", "#385E4C");
        styleSheet.replace("#C8E6C9", "#1C2922");
        styleSheet.replace("#E8F5E9", "#172119");
        styleSheet.replace("#F1F8E9", "#171E19");
        styleSheet.replace("#DCEDC8", "#192219");
        styleSheet.replace("#8BC34A", "#4B6D4A");
        styleSheet.replace("#A5D6A7", "#416D54");
        styleSheet.replace("#81C784", "#416D54");
        
        // 使用护眼色替换阅读器的文本颜色
        styleSheet.replace("QTextBrowser#readerTextBrowser {\n    font-family: \"Noto Serif\", \"Times New Roman\", serif;\n    font-size: 14pt;\n    line-height: 1.8;\n    color: #1C2833;\n    background-color: #FEF9E7;\n}", 
                          "QTextBrowser#readerTextBrowser {\n    font-family: \"Noto Serif\", \"Times New Roman\", serif;\n    font-size: 14pt;\n    line-height: 1.8;\n    color: #C8C8C8;\n    background-color: #121212;\n}");
                           
        // 滑块的夜间模式样式调整
        styleSheet.replace("QSlider::groove:horizontal {\n    border: none;\n    height: 8px;\n    background: #E0E0E0;",
                          "QSlider::groove:horizontal {\n    border: none;\n    height: 8px;\n    background: #2C2C2C;");
        
        // 调整滑块手柄的颜色
        styleSheet.replace("QSlider::handle:horizontal {\n    background: #42A5F5;",
                          "QSlider::handle:horizontal {\n    background: #3D6889;");
        
        // 添加dark-theme类属性
        this->setProperty("dark-theme", true);
        this->style()->unpolish(this);
        this->style()->polish(this);
        
        // 设置页码标签深色主题样式
        QString pageLabelStyle = QString(
            "QLabel {"
            "    color: #C8C8C8;"
            "    font-size: 12px;"
            "    font-weight: bold;"
            "    padding: 0 4px;"
            "}"
        );
        ui->currentPageLabel->setStyleSheet(pageLabelStyle);
        ui->totalPagesLabel->setStyleSheet(pageLabelStyle);
        ui->pageSeparatorLabel->setStyleSheet(pageLabelStyle);
        
    } else {
        ui->bookmarkComboBox->setStyleSheet(
            "QComboBox {"
            "    background-color: #FFFFFF;"
            "    color: #333333;"
            "    border: 1px solid #DDDDDD;"
            "    border-radius: 4px;"
            "    padding: 4px 8px;"
            "    min-height: 24px;"
            "}"
            "QComboBox:hover {"
            "    background-color: #F5F5F5;"
            "    border: 1px solid #2196F3;"
            "}"
            "QComboBox::drop-down {"
            "    border: none;"
            "    width: 20px;"
            "}"
            "QComboBox::down-arrow {"
            "    image: url(:/icons/down_arrow.svg);"
            "    width: 12px;"
            "    height: 12px;"
            "}"
            "QComboBox QAbstractItemView {"
            "    background-color: #FFFFFF;"
            "    color: #333333;"
            "    border: 1px solid #DDDDDD;"
            "    selection-background-color: #2196F3;"
            "    selection-color: #FFFFFF;"
            "}"
            );
        // 恢复浅色主题（如果之前是深色主题）
        styleSheet.replace("#121212", "#F9F9F9"); // 背景色
        styleSheet.replace("#1A1A1A", "#FFFFFF"); // 控件背景
        styleSheet.replace("#2C2C2C", "#DDDDDD"); // 边框色
        styleSheet.replace("#1C1C1C", "#F2F2F2"); // 次要背景
        styleSheet.replace("#252525", "#F0F0F0"); // 按钮悬停背景色
        styleSheet.replace("#1E1E1E", "#E5E5E5"); // 按钮按下背景色
        styleSheet.replace("#B0B0B0", "#333333"); // 主文本色
        styleSheet.replace("#A0A0A0", "#444444"); // 次要文本色
        styleSheet.replace("#909090", "#666666"); // 状态栏文本色
        styleSheet.replace("#3D6889", "#2196F3"); // 主题色
        styleSheet.replace("#2C5170", "#1976D2"); // 次要主题色
        styleSheet.replace("#141E26", "#E3F2FD"); // 选中背景色
        styleSheet.replace("#1E1E1E", "#F5F5F5"); // 悬浮背景色
        styleSheet.replace("#282828", "#EEEEEE"); // 分隔线颜色
        styleSheet.replace("#C0C0C0", "#1C2833"); // 阅读器文本颜色
        styleSheet.replace("#121212", "#FEF9E7"); // 阅读器背景颜色 (使用新的背景色)
        
        // 恢复绿色元素
        styleSheet.replace("#45705E", "#4CAF50");
        styleSheet.replace("#3E6455", "#43A047");
        styleSheet.replace("#385E4C", "#388E3C");
        styleSheet.replace("#1C2922", "#C8E6C9");
        styleSheet.replace("#172119", "#E8F5E9");
        styleSheet.replace("#171E19", "#F1F8E9");
        styleSheet.replace("#192219", "#DCEDC8");
        styleSheet.replace("#4B6D4A", "#8BC34A");
        styleSheet.replace("#416D54", "#A5D6A7");
        styleSheet.replace("#416D54", "#81C784");
        
        // 恢复阅读器的文本颜色 
        styleSheet.replace("QTextBrowser#readerTextBrowser {\n    font-family: \"Noto Serif\", \"Times New Roman\", serif;\n    font-size: 14pt;\n    line-height: 1.8;\n    color: #C8C8C8;\n    background-color: #121212;\n}", 
                          "QTextBrowser#readerTextBrowser {\n    font-family: \"Noto Serif\", \"Times New Roman\", serif;\n    font-size: 14pt;\n    line-height: 1.8;\n    color: #1C2833;\n    background-color: #FEF9E7;\n}");
        
        // 恢复滑块样式
        styleSheet.replace("QSlider::groove:horizontal {\n    border: none;\n    height: 8px;\n    background: #2C2C2C;",
                          "QSlider::groove:horizontal {\n    border: none;\n    height: 8px;\n    background: #E0E0E0;");
        
        // 恢复滑块手柄颜色
        styleSheet.replace("QSlider::handle:horizontal {\n    background: #3D6889;",
                          "QSlider::handle:horizontal {\n    background: #42A5F5;");
        
        // 移除dark-theme类属性
        this->setProperty("dark-theme", false);
        this->style()->unpolish(this);
        this->style()->polish(this);
        
        // 移除夜间模式特殊样式
        styleSheet = styleSheet.replace("#allBooksListWidget::item, #favPageListWidget::item, "
                          "#categoryListWidget::item { background-color: #1D1D1D; color: #B0B0B0; border-bottom: 1px solid #282828; }", "");
        styleSheet = styleSheet.replace("#allBooksListWidget::item:hover, #favPageListWidget::item:hover, "
                          "#categoryListWidget::item:hover { background-color: #2A2A2A; }", "");
        
        // 设置页码标签浅色主题样式
        QString pageLabelStyle = QString(
            "QLabel {"
            "    color: #333333;"
            "    font-size: 12px;"
            "    font-weight: bold;"
            "    padding: 0 4px;"
            "}"
        );
        ui->currentPageLabel->setStyleSheet(pageLabelStyle);
        ui->totalPagesLabel->setStyleSheet(pageLabelStyle);
        ui->pageSeparatorLabel->setStyleSheet(pageLabelStyle);
    }
    
    // 应用样式表
    qApp->setStyleSheet(styleSheet);
    
    // 为所有主要列表小部件应用夜间模式类
    QList<QListWidget*> listWidgets = findChildren<QListWidget*>();
    for (QListWidget* widget : listWidgets) {
        widget->setProperty("dark-theme", themeIndex == 1);
        widget->style()->unpolish(widget);
        widget->style()->polish(widget);
        
        // 强制刷新列表项目样式
        for(int i = 0; i < widget->count(); i++) {
            QListWidgetItem* item = widget->item(i);
            item->setSelected(item->isSelected()); // 触发重新应用样式
        }
    }
    
    // 强制立即更新所有阅读界面
    for (int i = 0; i < m_windows.size(); ++i) {
        if (m_windows[i].type == BOOK_READER) {
            // 更新背景颜色和文本颜色
            QString bgColor = themeIndex == 1 ? "#121212" : "#FEF9E7";
            QString textColor = themeIndex == 1 ? "#C8C8C8" : "#1C2833";
            
            // 创建强制覆盖样式
            QString style = QString(
                "QTextBrowser#readerTextBrowser {"
                "    background-color: %1 !important;"
                "    color: %2 !important;"
                "    border: none;"
                "    line-height: 1.8;"
                "    text-align: justify;"
                "    letter-spacing: 0.2px;"
                "    padding: 24px 32px;"
                "    border-radius: 8px;"
                "}"
            ).arg(bgColor, textColor);
            
            // 检查是否需要更新当前显示的阅读界面
            if (ui->contentStackedWidget->currentWidget() == ui->readerPage && 
                ui->windowListWidget->currentRow() == i) {
                QFont font = ui->readerTextBrowser->font();
                ui->readerTextBrowser->setFont(font); // 刷新样式
                ui->readerTextBrowser->setStyleSheet(style);
            }
        }
    }
    
    // 直接更新分类界面的颜色
    if (themeIndex == 1) {
        // 深色模式下的分类界面样式
        ui->categoryListWidget->setStyleSheet(
            "background-color: #1A1A1A; color: #C8C8C8; border: 1px solid #2C2C2C; border-radius: 8px;"
        );
        ui->categoriesListWidget->setStyleSheet(
            "background-color: #1A1A1A; color: #C8C8C8; border: 1px solid #2C2C2C; border-radius: 8px;"
        );
        // 设置我的书架背景颜色与分类一致
        ui->allBooksListWidget->setStyleSheet(
            "background-color: #1A1A1A; color: #C8C8C8; border: 1px solid #2C2C2C; border-radius: 8px;"
        );
        ui->favPageListWidget->setStyleSheet(
            "background-color: #1A1A1A; color: #C8C8C8; border: 1px solid #2C2C2C; border-radius: 8px;"
        );
        ui->categoryTitleLabel->setStyleSheet("color: #C8C8C8; font-weight: bold;");
        
        // 设置分类页内的其他控件样式
        ui->categorySearchLineEdit->setStyleSheet(
            "background-color: #252525; color: #C8C8C8; border: 1px solid #2C2C2C; border-radius: 4px; padding: 4px;"
        );
        ui->categorySearchButton->setStyleSheet(
            "background-color: #252525; border: 1px solid #2C2C2C; border-radius: 4px;"
        );
    } else {
        // 移除所有自定义样式，恢复默认样式
        ui->categoryListWidget->setStyleSheet("");
        ui->categoriesListWidget->setStyleSheet("");
        ui->allBooksListWidget->setStyleSheet("");
        ui->favPageListWidget->setStyleSheet("");
        ui->categoryTitleLabel->setStyleSheet("");
        ui->categorySearchLineEdit->setStyleSheet("");
        ui->categorySearchButton->setStyleSheet("");
    }
    
    // 强制重新应用样式到所有打开的阅读界面
    // 这是为了解决切换主题后已经打开的阅读界面颜色不更新的问题
    if (ui->contentStackedWidget->currentWidget() == ui->readerPage) {
        QString bgColor = themeIndex == 1 ? "#121212" : "#FEF9E7";
        QString textColor = themeIndex == 1 ? "#C8C8C8" : "#1C2833";
        
        QString style = QString(
            "QTextBrowser#readerTextBrowser {"
            "    background-color: %1 !important;"
            "    color: %2 !important;"
            "    border: none;"
            "    line-height: 1.8;"
            "    text-align: justify;"
            "    letter-spacing: 0.2px;"
            "    padding: 24px 32px;"
            "    border-radius: 8px;"
            "}"
        ).arg(bgColor, textColor);
        
        ui->readerTextBrowser->setStyleSheet(style);
        
        // 强制刷新
        ui->readerTextBrowser->repaint();
    }
}

void MainWindow::setupReaderNavigation()
{

    // 设置页面滑块
    ui->pageSlider->setMinimum(1);
    ui->pageSlider->setMaximum(100);
    ui->pageSlider->setValue(1);
    ui->pageSlider->setStyleSheet(
        "QSlider::handle:horizontal {"
        "    width: 20px;"
        "    height: 20px;"
        "    margin: -6px 0;"
        "    border-radius: 10px;"
        "    background: qradialgradient(cx:0.5, cy:0.5, radius:0.5, fx:0.5, fy:0.5, stop:0 #FFFFFF, stop:1 #2196F3);"
        "    border: 2px solid #1976D2;"
        "}"
        "QSlider::handle:horizontal:hover {"
        "    background: qradialgradient(cx:0.5, cy:0.5, radius:0.5, fx:0.5, fy:0.5, stop:0 #FFFFFF, stop:1 #42A5F5);"
        "}"
    );
    ui->pageSlider->setToolTip(tr("阅读进度"));
    

    // 应用动画效果
    QGraphicsOpacityEffect *opacityEffect = new QGraphicsOpacityEffect(ui->readerToolbarWidget);
    ui->readerToolbarWidget->setGraphicsEffect(opacityEffect);
    
    // 连接信号

    connect(ui->pageSlider, &QSlider::valueChanged, this, &MainWindow::on_pageSlider_valueChanged);
    updateBookmarkComboBox(); // 初始化书签下拉框

    // 禁用文本浏览器的滚动条
    ui->readerTextBrowser->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->readerTextBrowser->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // 创建并设置章节切换按钮
    QPushButton *prevChapterButton = new QPushButton("<", ui->readerToolbarWidget);
    QPushButton *nextChapterButton = new QPushButton(">", ui->readerToolbarWidget);
    
    prevChapterButton->setObjectName("prevChapterButton");
    nextChapterButton->setObjectName("nextChapterButton");
    
    // 设置按钮样式
    QString buttonStyle = 
        "QPushButton {"
        "    background-color: #2196F3;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 4px;"
        "    padding: 4px 8px;"
        "    min-width: 30px;"
        "    max-width: 30px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #1976D2;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #0D47A1;"
        "}";
    
    prevChapterButton->setStyleSheet(buttonStyle);
    nextChapterButton->setStyleSheet(buttonStyle);
    
    // 设置工具提示
    prevChapterButton->setToolTip(tr("上一章"));
    nextChapterButton->setToolTip(tr("下一章"));
    
    // 将按钮添加到布局中
    QHBoxLayout *layout = qobject_cast<QHBoxLayout*>(ui->readerToolbarWidget->layout());
    if (layout) {
        // 在页面滑块两侧插入章节按钮
        layout->insertWidget(layout->indexOf(ui->pageSlider), prevChapterButton);
        layout->insertWidget(layout->indexOf(ui->pageSlider) + 2, nextChapterButton);
    }
    
    // 连接信号
    connect(prevChapterButton, &QPushButton::clicked, this, &MainWindow::gotoPreviousChapter);
    connect(nextChapterButton, &QPushButton::clicked, this, &MainWindow::gotoNextChapter);
}

void MainWindow::on_pageSlider_valueChanged(int value)
{
    // 滑块变化时更新阅读位置
    /*
    QScrollBar* scrollBar = ui->readerTextBrowser->verticalScrollBar();
    int maxValue = scrollBar->maximum();
    if (maxValue > 0) {
        int scrollPos = (value * maxValue) / 100;
        scrollBar->setValue(scrollPos);
    }
    */

    if (zIsScorll)
    {
        return;
    }

    if (zChapterDocument && !zCurrentChapterId.isEmpty())
    {
        goToPage(value);
    }
}

void MainWindow::gotoPreviousPage()
{
    //上一页
    if (zChapterDocument && !zCurrentChapterId.isEmpty() && zCurrentPage > 1)
    {
        goToPage(zCurrentPage - 1);
    }
}

void MainWindow::gotoNextPage()
{
    //下一页
    if (zChapterDocument && !zCurrentChapterId.isEmpty() && zCurrentPage < zTotalPage)
    {
        goToPage(zCurrentPage + 1);
    }
}

void MainWindow::gotoPreviousChapter()
{
    if (zCurrentBookItemIndex > 0) {
        loadChapter(zCurrentBookSpineId[zCurrentBookItemIndex - 1]);
    }
}

void MainWindow::gotoNextChapter()
{
    if (zCurrentBookItemIndex < zCurrentBookSpineId.size() - 1) {
        loadChapter(zCurrentBookSpineId[zCurrentBookItemIndex + 1]);
    }
}

void MainWindow::loadChapter(const QString& itemId)
{
    if (!zEpubParser)
    {
        qWarning() << "zEpubParser is NULL";
        zChapterDocument->setHtml("<p>错误：EPUB解析器未被正确初始化。</p>");
        zCurrentChapterId.clear();
        updatePagination();
        goToPage(1);
        return;
    }

    if (zCurrentBookFikePath.isEmpty())
    {
        qWarning() << "zCurrentBookFilePath is NULL";
        zChapterDocument->setHtml("<p>错误：当前未打开EPUB书籍</p>");
        updatePagination();
        goToPage(1);
        return;
    }

    zCurrentChapterId = itemId;
    QString chapterHtml = zEpubParser->getContentById(itemId);

    if (chapterHtml.isEmpty())
    {
        QString errorMessage = zEpubParser->getLastError();
        if (!errorMessage.isEmpty())
        {
            qWarning() << "getContentById failed (error):" << errorMessage;
            chapterHtml = tr("<p>加载章节 '%1' 失败: %2</p>").arg(itemId).arg(errorMessage);
        }
        else 
        {
            qWarning() << "getContentById failed";
            chapterHtml = tr("<p>章节 '%1' 为空").arg(itemId);
        }
    }

    zIsScorll = true;

    QFont currentFont = zChapterDocument->defaultFont();
    currentFont.setPointSize(m_currentFontSize);
    zChapterDocument->setDefaultFont(currentFont);

    zChapterDocument->setHtml(chapterHtml);

    updatePagination();

    goToPage(1);//默认第一页，要改

    zCurrentBookItemIndex = zCurrentBookSpineId.indexOf(itemId);//更新索引

    zIsScorll = false;

    QGraphicsOpacityEffect* effect = qobject_cast<QGraphicsOpacityEffect*>(ui->readerTextBrowser->graphicsEffect());
    if (effect) 
    {
        effect->setOpacity(0.0);
        QPropertyAnimation* animation = new QPropertyAnimation(effect, "opacity", this);
        animation->setDuration(300);
        animation->setStartValue(0.0);
        animation->setEndValue(1.0);
        animation->setEasingCurve(QEasingCurve::InOutQuad);
        animation->start(QAbstractAnimation::DeleteWhenStopped);
    }

    // 更新章节按钮状态
    QPushButton *prevChapterButton = findChild<QPushButton*>("prevChapterButton");
    QPushButton *nextChapterButton = findChild<QPushButton*>("nextChapterButton");
    
    if (prevChapterButton && nextChapterButton) {
        prevChapterButton->setEnabled(zCurrentBookItemIndex > 0);
        nextChapterButton->setEnabled(zCurrentBookItemIndex < zCurrentBookSpineId.size() - 1);
    }
}

void MainWindow::updatePagination()
{
    if (!zChapterDocument || !ui->readerTextBrowser->viewport())
    {
        return;
    }

    QFont docFont = zChapterDocument->defaultFont();
    if (docFont.pointSize() != m_currentFontSize)
    {
        docFont.setPointSize(m_currentFontSize);
        zChapterDocument->setDefaultFont(docFont);
    }

    QSizeF viewPointSize = ui->readerTextBrowser->viewport()->size();
    if (viewPointSize.height() <= 0 || viewPointSize.width() <= 0)
    {
        zTotalPage = 1;//至少一页
    }
    else
    {
        zChapterDocument->setPageSize(viewPointSize);
        zTotalPage = zChapterDocument->pageCount();
        if (zTotalPage == 0)
        {
            zTotalPage = 1;//至少一页
        }
    }

    if (zCurrentPage > zTotalPage)
    {
        zCurrentPage = zTotalPage;//保证当前页数在总页数范围内
    }
    if (zCurrentPage < 1)
    {
        zCurrentPage = 1;//确保从第一页开始
    }

    zIsScorll = true;
    ui->pageSlider->setRange(1, zTotalPage);
    ui->pageSlider->setValue(zCurrentPage);
    ui->pageSlider->setEnabled(zTotalPage > 1);
    zIsScorll = false;

    // 更新页码显示
    ui->currentPageLabel->setText(QString::number(zCurrentPage));
    ui->totalPagesLabel->setText(QString::number(zTotalPage));
}

void MainWindow::goToPage(int pageNum)
{
    if (!zChapterDocument || zTotalPage == 0)
    {
        return;
    }

    int targetPage = qBound(1, pageNum, zTotalPage);//确保在范围内

    zCurrentPage = targetPage;

    qreal PageHight = zChapterDocument->pageSize().height();
    if (PageHight <= 0)
    {
        if (ui->pageSlider->value() != zCurrentPage)
        {
            zIsScorll = true;
            ui->pageSlider->setValue(zCurrentPage);
            zIsScorll = false;
        }
        return;
    }

    int scrollPos = qRound((zCurrentPage - 1) * PageHight);

    zIsScorll = true;
    ui->readerTextBrowser->verticalScrollBar()->setValue(scrollPos);
    if (ui->pageSlider->value() != zCurrentPage)
    {
        ui->pageSlider->setValue(zCurrentPage);
    }
    zIsScorll = false;

    //更新页码
    ui->currentPageLabel->setText(QString::number(zCurrentPage));
}

void MainWindow::onReaderScroll()
{
    if (zIsScorll || !zChapterDocument || zCurrentChapterId.isEmpty())
    {
        return;
    }

    qreal PageHeight = zChapterDocument->pageSize().height();
    if (PageHeight <= 0)
    {
        return;
    }

    int currentScrollPos = ui->readerTextBrowser->verticalScrollBar()->value();

    int newPage = qRound(static_cast<qreal>(currentScrollPos) / PageHeight) + 1;

    newPage = qBound(1, newPage, zTotalPage);

    if (newPage != zCurrentPage)
    {
        zCurrentPage = newPage;
        zIsScorll = true;
        ui->pageSlider->setValue(newPage);
        zIsScorll = false;
    }
    //更新页码
    ui->currentPageLabel->setText(QString::number(zCurrentPage));
}

// 更新书签下拉框
void MainWindow::updateBookmarkComboBox()
{
    ui->bookmarkComboBox->clear();
    ui->bookmarkComboBox->addItem("选择书签",QVariant()); // 添加默认选项
    
    QString currentFilePath = zCurrentBookFikePath;
    if (currentFilePath.isEmpty()) return;
    
    auto it = allBooks.find(currentFilePath);
    if (it != allBooks.end()) {
        const auto& bookmarks = it->BookMarks;
        //// 将书签按页码排序
        //QList<int> pages = bookmarks.keys();
        //std::sort(pages.begin(), pages.end());
        //for (int page : pages) {
        //    ui->bookmarkComboBox->addItem(QString("第 %1 页").arg(page), page);
        //}
        QList<bookMark> sortBookMark = bookmarks;
        std::sort(sortBookMark.begin(), sortBookMark.end(), [&](const bookMark& bookm1, const bookMark& bookm2) {
            int index1 = zCurrentBookSpineId.indexOf(bookm1.chapterId);
            int index2 = zCurrentBookSpineId.indexOf(bookm2.chapterId);

            if (index1 != index2)
                return index1 < index2;
            
            return bookm1.pageInChapter < bookm2.pageInChapter;
            });
            
        for (const auto& bookmark : sortBookMark)
        {
            QString itemText = QString("%1 第 %2 页").arg(bookmark.chapterTitle).arg(bookmark.pageInChapter);
            QVariantMap bookMarkData;
            bookMarkData["chapterId"] = bookmark.chapterId;
            bookMarkData["pageInChapter"] = bookmark.pageInChapter;
            ui->bookmarkComboBox->addItem(itemText, QVariant::fromValue(bookMarkData));
        }
    }
   
}

// 处理书签选择
void MainWindow::on_bookmarkComboBox_currentIndexChanged(int index)
{
    if (index <= 0) return; // 忽略默认选项
    
    /*int page = ui->bookmarkComboBox->currentData().toInt();
    QString chapter = ui->bookmarkComboBox->currentData().toString();*/
    QVariant markData = ui->bookmarkComboBox->itemData(index);
    if (markData.canConvert<QVariantMap>())
    {
        QVariantMap markDataMap = markData.toMap();
        QString chapter = markDataMap.value("chapterId").toString();
        int page = markDataMap.value("pageInChapter").toInt();
        
        if (chapter != zCurrentChapterId)
        {
            loadChapter(chapter);
        }

        if (chapter == zCurrentChapterId)
        {
            QTimer::singleShot(0, this, [this, page]() {
                goToPage(page);
                });
        }
    }
}

// 保存阅读记录
void MainWindow::saveReadingRecord(const QString& filePath) {
    if (allBooks.contains(filePath)) {
        BookInfo& book = allBooks[filePath];
        book.lastReadRecord.chapterId = zCurrentChapterId;
        book.lastReadRecord.chapterTitle = zChapterDocument->metaInformation(QTextDocument::DocumentTitle);
        book.lastReadRecord.pageInChapter = zCurrentPage;

        QFileInfo fileInfo(filePath);
        QString categoryDir;
        for (const QString& category : book.categories) {
            if (m_categories.contains(category)) {
                categoryDir = QDir::currentPath() + "/" + category;
                break;
            }
        }
        if (!categoryDir.isEmpty()) {
            QString bookDir = categoryDir + "/" + fileInfo.baseName();
            QDir().mkpath(bookDir);
            QString recordFilePath = bookDir + "/record";
            QFile recordFile(recordFilePath);
            if (recordFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&recordFile);
                out << book.lastReadRecord.chapterId << "\n";
                out << book.lastReadRecord.chapterTitle << "\n";
                out << book.lastReadRecord.pageInChapter;
                recordFile.close();
            }
        }
    }
}

// 加载阅读记录
void MainWindow::loadReadingRecord(const QString& filePath) {
    if (allBooks.contains(filePath)) {
        BookInfo& book = allBooks[filePath];
        QFileInfo fileInfo(filePath);
        QString categoryDir;
        for (const QString& category : book.categories) {
            if (m_categories.contains(category)) {
                categoryDir = QDir::currentPath() + "/" + category;
                break;
            }
        }
        if (!categoryDir.isEmpty()) {
            QString bookDir = categoryDir + "/" + fileInfo.baseName();
            QString recordFilePath = bookDir + "/record";
            QFile recordFile(recordFilePath);
            if (recordFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&recordFile);
                book.lastReadRecord.chapterId = in.readLine();
                book.lastReadRecord.chapterTitle = in.readLine();
                book.lastReadRecord.pageInChapter = in.readLine().toInt();
                recordFile.close();

                // 跳转到最后阅读位置
                loadChapter(book.lastReadRecord.chapterId);
                goToPage(book.lastReadRecord.pageInChapter);
            }
        }
    }
}

// 保存书签信息
void MainWindow::saveBookmarkInfo(const QString& filePath) {
    if (allBooks.contains(filePath)) {
        BookInfo& book = allBooks[filePath];
        QFileInfo fileInfo(filePath);
        QString categoryDir;
        for (const QString& category : book.categories) {
            if (m_categories.contains(category)) {
                categoryDir = QDir::currentPath() + "/" + category;
                break;
            }
        }
        if (!categoryDir.isEmpty()) {
            QString bookDir = categoryDir + "/" + fileInfo.baseName();
            QDir().mkpath(bookDir);
            QString bookmarkFilePath = bookDir + "/bookmarkmessage";
            QFile bookmarkFile(bookmarkFilePath);
            if (bookmarkFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&bookmarkFile);
                for (const bookMark& bookmark : book.BookMarks) {
                    out << bookmark.chapterId << "\n";
                    out << bookmark.chapterTitle << "\n";
                    out << bookmark.pageInChapter << "\n";
                }
                bookmarkFile.close();
            }
        }
    }
}

void MainWindow::saveBookmarkInfo(const QString& filePath) {
    // 检查书籍是否存在于存储列表中
    if (!allBooks.contains(filePath)) return;
    
    // 获取书籍信息
    BookInfo& book = allBooks[filePath];
    QFileInfo fileInfo(filePath); // 获取文件路径信息（用于提取书名）
    
    // 查找书籍所属分类（取第一个分类，可根据需求扩展）
    QString categoryDir;
    if (!book.categories.isEmpty() && m_categories.contains(book.categories.first())) {
        categoryDir = QDir::currentPath() + "/" + book.categories.first(); // 分类目录路径
    } else {
        qWarning() << "not found category!";
        return;
    }
    
    // 创建书籍目录（分类/书名）
    QString bookDir = categoryDir + "/" + fileInfo.baseName(); // 书名目录路径
    QDir().mkpath(bookDir); // 自动创建多级目录（若不存在）
    
    // 书签文件路径
    QString bookmarkFilePath = bookDir + "/bookmarkmessage";
    QFile bookmarkFile(bookmarkFilePath); // 创建文件对象
    
    // 写入书签数据
    if (bookmarkFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&bookmarkFile); // 文本流写入
        for (const bookMark& bookmark : book.BookMarks) {
            // 按行写入：chapterId、chapterTitle、pageInChapter
            out << bookmark.chapterId << "\n";
            out << bookmark.chapterTitle << "\n";
            out << bookmark.pageInChapter << "\n";
        }
        bookmarkFile.close();
        qDebug() << "Succeed!：" << bookmarkFilePath;
    } else {
        qWarning() << "Failed!：" << bookmarkFile.errorString();
    }
}
