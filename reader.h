#ifndef READER_H
#define READER_H

#include <QMainWindow>
#include <QListWidgetItem>
#include <QMap>
#include <QString>
#include <QTime>
#include <QDateTime>
#include <QFileInfo>
#include <QIcon>
#include <QMenu>
#include <QInputDialog>
#include "readerform.h"
#include <QTextDocument>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

// 窗口类型枚举
enum WindowType {
    MAIN_WINDOW,    // 主窗口
    FAVORITES,      // 收藏夹
    CATEGORY,       // 分类
    BOOK_READER     // 书籍阅读器
};

// 表示电子书的数据结构
struct BookInfo {
    QString filePath;        // 文件路径
    QString title;           // 书籍标题
    QTime totalReadTime;     // 总阅读时间
    bool isFavorite;         // 是否是收藏
    QDateTime lastReadTime;  // 最后阅读时间
    QList<QString> categories; // 所属分类
    QMap<int, QString> bookmarks; // 书签列表，键为页码，值为书签描述
    
    BookInfo() : isFavorite(false) {
        totalReadTime = QTime(0, 0);
    }
};

// 分类信息
struct CategoryInfo {
    QString name;            // 分类名称
    QList<QString> books;    // 该分类下的书籍路径
    
    CategoryInfo(const QString &n = QString()) : name(n) {}
};

// 窗口信息
struct WindowInfo {
    WindowType type;         // 窗口类型
    QString title;           // 标题
    QString identifier;      // 标识符（文件路径或分类名称）
    
    WindowInfo(WindowType t, const QString &ttl, const QString &id = QString())
        : type(t), title(ttl), identifier(id) {}
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // 窗口选择相关
    void on_windowListWidget_itemClicked(QListWidgetItem *item);
    
    // 按钮相关
    void on_addBookButton_clicked();
    void on_sortByNameButton_clicked();
    void on_sortByTimeButton_clicked();
    void on_sortByRecentButton_clicked();
    void on_closeBookButton_clicked();
    void on_bookmarkButton_clicked();
    void on_addToButton_clicked();
    void on_addCategoryButton_clicked();
    void on_searchButton_clicked();
    void on_favSearchButton_clicked();
    void on_categorySearchButton_clicked();
    
    // 列表项操作
    void on_allBooksListWidget_itemDoubleClicked(QListWidgetItem *item);
    void on_categoriesListWidget_itemDoubleClicked(QListWidgetItem *item);
    void on_favPageListWidget_itemDoubleClicked(QListWidgetItem *item);
    void on_categoryListWidget_itemDoubleClicked(QListWidgetItem *item);

    // 窗口列表右键菜单
    void onWindowListContextMenu(const QPoint &pos);
    void onCategoriesListContextMenu(const QPoint &pos);
    
    // 搜索功能
    void on_searchLineEdit_textChanged(const QString &text);
    void on_favSearchLineEdit_textChanged(const QString &text);
    void on_categorySearchLineEdit_textChanged(const QString &text);

    void on_themeButton_clicked();
    
    void on_pageSlider_valueChanged(int value);

    //滚动条
    void onReaderScroll();

    void on_bookmarkComboBox_currentIndexChanged(int index);

private:
    Ui::MainWindow *ui;

    
    readerform* zEpubParser;//指向epub解析的指针
    
    QString zCurrentBookFikePath;//当前打开书的路径

    QString zCurrentChapterId;//当前章节的id

    QTextDocument* zChapterDocument;//文档对象

    QList<QString> zCurrentBookSpineId;//章节id列表
    int zCurrentBookItemIndex;//章节索引

    int zCurrentPage;//当前页码
    int zTotalPage;//总页数
    bool zIsScorll;//防止滑动和滚动递归触发

    // 存储所有电子书信息
    QMap<QString, BookInfo> allBooks;
    
    // 存储分类信息
    QMap<QString, CategoryInfo> m_categories;
    
    // 存储当前打开的窗口信息
    QList<WindowInfo> m_windows;
    
    // 当前的排序方法
    int m_sortMethod;
    
    // 当前主题 (0: 浅色, 1: 深色)
    int m_currentTheme;
    
    // 初始化UI
    void setupUI();
    
    // 刷新书籍列表
    void refreshBookLists();
    
    // 添加书籍到UI列表
    void addBookToList(QListWidget *listWidget, const BookInfo &book);
    
    // 按指定方式对书籍排序
    void sortBooks(int sortMethod);
    
    // 加载示例数据（实际应用中会从文件加载）
    void loadSampleBooks();
    
    // 打开电子书进行阅读
    void openBook(const QString &filePath);
    
    // 添加窗口到窗口列表
    void addWindow(WindowType type, const QString &title, const QString &identifier = QString());
    
    // 删除窗口
    void removeWindow(int index);
    
    // 切换到指定窗口
    void switchToWindow(int index);
    
    // 初始化窗口列表
    void initWindowList();
    
    // 更新收藏夹页面
    void updateFavoritesPage();
    
    // 更新分类页面
    void updateCategoryPage(const QString &categoryName);
    
    // 创建分类
    void createCategory(const QString &name);
    
    // 刷新分类列表
    void refreshCategoriesList();
    
    // 添加书籍到分类
    void addBookToCategory(const QString &filePath, const QString &categoryName);
    
    // 显示/隐藏搜索框
    void toggleSearchVisibility(QLineEdit *searchEdit, bool visible);
    
    // 搜索书籍
    void searchBooks(QListWidget *listWidget, const QString &text, const QList<QString> &bookPaths);
    
    // 获取某个分类或收藏夹中的书籍
    QList<QString> getBooksInCategory(const QString &categoryName);
    QList<QString> getFavoriteBooks();
    
    // 切换主题
    void switchTheme(int themeIndex);
    
    // 阅读器相关函数
    void setupReaderNavigation();
    int m_currentFontSize;
    void gotoPreviousPage();
    void gotoNextPage();


    void loadChapter(const QString& itemId);//载入章节
    void goToPage(int pageNum);//跳转
    void updatePagination();//计算页数

    void updateBookmarkComboBox();

    // QObject interface
public:
    virtual bool eventFilter(QObject *watched, QEvent *event) override;
};


#endif // READER_H
