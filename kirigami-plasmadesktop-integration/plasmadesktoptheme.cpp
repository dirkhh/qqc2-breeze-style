/*
    SPDX-FileCopyrightText: 2017 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2020 Noah Davis <noahadvs@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "plasmadesktoptheme.h"
#include <QQmlEngine>
#include <QQmlContext>
#include <QGuiApplication>
#include <QPalette>
#include <QDebug>
#include <QQuickWindow>
#include <QScreen>
#include <KIconLoader>
#include <KColorUtils>
#include <KColorScheme>
#include <KConfigGroup>
#include <QDBusConnection>

#include <cmath>

class IconLoaderSingleton
{
public:
    IconLoaderSingleton() = default;

    KIconLoader self;
};

Q_GLOBAL_STATIC(IconLoaderSingleton, privateIconLoaderSelf)

class StyleSingleton : public QObject
{
    Q_OBJECT

public:
    struct Colors {
        QPalette palette;
        KColorScheme selectionScheme;
        KColorScheme scheme;
    };

    explicit StyleSingleton()
        : QObject()
        , buttonScheme(QPalette::Active, KColorScheme::ColorSet::Button)
        , viewScheme(QPalette::Active, KColorScheme::ColorSet::View)
    {
        connect(qGuiApp, &QGuiApplication::paletteChanged,
                this, &StyleSingleton::refresh);

        // Use DBus in order to listen for kdeglobals changes directly, as the
        // QApplication doesn't expose the font variants we're looking for,
        // namely smallFont.
        QDBusConnection::sessionBus().connect( QString(),
        QStringLiteral( "/KGlobalSettings" ),
        QStringLiteral( "org.kde.KGlobalSettings" ),
        QStringLiteral( "notifyChange" ), this, SIGNAL(configurationChanged()));
    }

    void refresh()
    {
        m_cache.clear();
        buttonScheme = KColorScheme(QPalette::Active, KColorScheme::ColorSet::Button);
        viewScheme = KColorScheme(QPalette::Active, KColorScheme::ColorSet::View);

        Q_EMIT paletteChanged();
    }

    Colors loadColors(Kirigami::PlatformTheme::ColorSet cs, QPalette::ColorGroup group)
    {
        const auto key = qMakePair(cs, group);
        auto it = m_cache.constFind(key);
        if (it != m_cache.constEnd())
            return *it;

        using Kirigami::PlatformTheme;

        KColorScheme::ColorSet set;

        switch (cs) {
        case PlatformTheme::Button:
            set = KColorScheme::ColorSet::Button;
            break;
        case PlatformTheme::Selection:
            set = KColorScheme::ColorSet::Selection;
            break;
        case PlatformTheme::Tooltip:
            set = KColorScheme::ColorSet::Tooltip;
            break;
        case PlatformTheme::View:
            set = KColorScheme::ColorSet::View;
            break;
        case PlatformTheme::Complementary:
            set = KColorScheme::ColorSet::Complementary;
            break;
        case PlatformTheme::Header:
            set = KColorScheme::ColorSet::Header;
            break;
        case PlatformTheme::Window:
        default:
            set = KColorScheme::ColorSet::Window;
        }

        // HACK/FIXME: Working around the fact that KColorScheme changes the selection background color when inactive by default.
        // Yes, this is horrible.
        QPalette::ColorGroup selectionGroup = group == QPalette::Inactive ? QPalette::Active : group;
        Colors ret = {{}, KColorScheme(selectionGroup, KColorScheme::ColorSet::Selection), KColorScheme(group, set)};

        QPalette pal;
        for (auto state : { QPalette::Active, QPalette::Inactive, QPalette::Disabled }) {
            pal.setBrush(state, QPalette::WindowText, ret.scheme.foreground());
            pal.setBrush(state, QPalette::Window, ret.scheme.background());
            pal.setBrush(state, QPalette::Base, ret.scheme.background());
            pal.setBrush(state, QPalette::Text, ret.scheme.foreground());
            pal.setBrush(state, QPalette::Button, ret.scheme.background());
            pal.setBrush(state, QPalette::ButtonText, ret.scheme.foreground());
            pal.setBrush(state, QPalette::Highlight, ret.selectionScheme.background());
            pal.setBrush(state, QPalette::HighlightedText, ret.selectionScheme.foreground());
            pal.setBrush(state, QPalette::ToolTipBase, ret.scheme.background());
            pal.setBrush(state, QPalette::ToolTipText, ret.scheme.foreground());

            pal.setColor(state, QPalette::Light, ret.scheme.shade(KColorScheme::LightShade));
            pal.setColor(state, QPalette::Midlight, ret.scheme.shade(KColorScheme::MidlightShade));
            pal.setColor(state, QPalette::Mid, ret.scheme.shade(KColorScheme::MidShade));
            pal.setColor(state, QPalette::Dark, ret.scheme.shade(KColorScheme::DarkShade));
            pal.setColor(state, QPalette::Shadow, QColor(0,0,0,51 /* 20% */));//ret.scheme.shade(KColorScheme::ShadowShade));

            pal.setBrush(state, QPalette::AlternateBase, ret.scheme.background(KColorScheme::AlternateBackground));
            pal.setBrush(state, QPalette::Link, ret.scheme.foreground(KColorScheme::LinkText));
            pal.setBrush(state, QPalette::LinkVisited, ret.scheme.foreground(KColorScheme::VisitedText));

            pal.setBrush(state, QPalette::PlaceholderText, ret.scheme.foreground(KColorScheme::InactiveText));
            pal.setBrush(state, QPalette::BrightText, KColorUtils::hcyColor(
                KColorUtils::hue(pal.buttonText().color()),KColorUtils::chroma(pal.buttonText().color()), 1 - KColorUtils::luma(pal.buttonText().color())
            ));
        }
        ret.palette = pal;
        m_cache.insert(key, ret);
        return ret;
    }

    KColorScheme buttonScheme;
    KColorScheme viewScheme;

Q_SIGNALS:
    void configurationChanged();
    void paletteChanged();

private:
    QHash<QPair<Kirigami::PlatformTheme::ColorSet, QPalette::ColorGroup>, Colors> m_cache;
};
Q_GLOBAL_STATIC_WITH_ARGS(QScopedPointer<StyleSingleton>, s_style, (new StyleSingleton))

PlasmaDesktopTheme::PlasmaDesktopTheme(QObject *parent)
    : PlatformTheme(parent)
{
    m_lowPowerHardware = QByteArrayList{"1", "true"}.contains(qgetenv("KIRIGAMI_LOWPOWER_HARDWARE").toLower());

    qreal scaleFactor = qGuiApp->devicePixelRatio();
    qreal scaleFactorRemainder = fmod(scaleFactor, 1.0);

    /* QtTextRendering uses less memory, so use it in low power mode.
     * 
     * For scale factors greater than 2, native rendering doesn't actually do much.
     * Does native rendering even work when scaleFactor >= 2?
     * 
     * NativeTextRendering is still distorted sometimes with fractional scale
     * factors, despite https://bugreports.qt.io/browse/QTBUG-67007 being closed.
     * 1.5x scaling looks generally OK, but there are occasional and difficult to
     * reproduce issues with all fractional scale factors.
     */
    QQuickWindow::TextRenderType defaultTextRenderType = !m_lowPowerHardware
        && scaleFactor < 2
        && (scaleFactorRemainder == 0.0 /*|| scaleFactor == 1.5*/)
        ? QQuickWindow::NativeTextRendering : QQuickWindow::QtTextRendering;

    // Allow setting the text rendering type with an environment variable
    QByteArrayList validInputs = {"qttextrendering", "qtrendering", "nativetextrendering", "nativerendering"};
    QByteArray input = qgetenv("QT_QUICK_DEFAULT_TEXT_RENDER_TYPE").toLower();
    if (validInputs.contains(input)) {
        if (input == validInputs[0] || input == validInputs[1]) {
            defaultTextRenderType = QQuickWindow::QtTextRendering;
        } else {
            defaultTextRenderType = QQuickWindow::NativeTextRendering;
        }
    }

    QQuickWindow::setTextRenderType(defaultTextRenderType);

    //---------------------------

    setSupportsIconColoring(true);
    m_parentItem = qobject_cast<QQuickItem *>(parent);

    //null in case parent is a normal QObject
    if (m_parentItem) {
        connect(m_parentItem.data(), &QQuickItem::enabledChanged,
                this, &PlasmaDesktopTheme::syncColors);
        if (m_parentItem && m_parentItem->window()) {
            connect(m_parentItem->window(), &QWindow::activeChanged,
                    this, &PlasmaDesktopTheme::syncColors);
            m_window = m_parentItem->window();
        }
        connect(m_parentItem.data(), &QQuickItem::windowChanged,
                this, [this]() {
                    if (m_window) {
                        disconnect(m_window.data(), &QWindow::activeChanged,
                                this, &PlasmaDesktopTheme::syncColors);
                    }
                    if (m_parentItem && m_parentItem->window()) {
                        connect(m_parentItem->window(), &QWindow::activeChanged,
                                this, &PlasmaDesktopTheme::syncColors);
                    }
                    syncColors();
                });
    }

    //TODO: correct? depends from https://codereview.qt-project.org/206889
    connect(qGuiApp, &QGuiApplication::fontDatabaseChanged, this, [this]() {setDefaultFont(qApp->font());});
    configurationChanged();

    connect(this, &PlasmaDesktopTheme::colorSetChanged,
            this, &PlasmaDesktopTheme::syncColors);
    connect(this, &PlasmaDesktopTheme::colorGroupChanged,
            this, &PlasmaDesktopTheme::syncColors);

    connect(s_style->data(), &StyleSingleton::paletteChanged,
            this, &PlasmaDesktopTheme::syncColors);

    connect(s_style->data(), &StyleSingleton::configurationChanged,
            this, &PlasmaDesktopTheme::configurationChanged);

    syncColors();
}

PlasmaDesktopTheme::~PlasmaDesktopTheme() = default;

void PlasmaDesktopTheme::configurationChanged()
{
    KSharedConfigPtr ptr = KSharedConfig::openConfig();
    KConfigGroup general( ptr->group("general") );
    setSmallFont(general.readEntry("smallestReadableFont", []() {
        auto smallFont = qApp->font();
        if (smallFont.pixelSize() != -1) {
            smallFont.setPixelSize(smallFont.pixelSize()-2);
        } else {
            smallFont.setPointSize(smallFont.pointSize()-2);
        }
        return smallFont;
    }()));
}

QIcon PlasmaDesktopTheme::iconFromTheme(const QString &name, const QColor &customColor)
{
    QPalette pal = palette();
    if (customColor != Qt::transparent) {
        for (auto state : { QPalette::Active, QPalette::Inactive, QPalette::Disabled }) {
            pal.setBrush(state, QPalette::WindowText, customColor);
        }
    }

    privateIconLoaderSelf->self.setCustomPalette(pal);

    return KDE::icon(name, &privateIconLoaderSelf->self);
}

void PlasmaDesktopTheme::syncColors()
{
    QPalette::ColorGroup group = (QPalette::ColorGroup)colorGroup();
    if (m_parentItem) {
        if (!m_parentItem->isEnabled()) {
            group = QPalette::Disabled;
        //Why also checking the window is exposed?
        //in the case of QQuickWidget the window() will never be active
        //and the widgets will always have the inactive palette.
        // better to always show it active than always show it inactive
        } else if (m_parentItem->window() && !m_parentItem->window()->isActive() && m_parentItem->window()->isExposed()) {
            group = QPalette::Inactive;
        }
    }

    const auto colors = (*s_style)->loadColors(colorSet(), group);
    setPalette(colors.palette);

    //foreground
    setTextColor(colors.scheme.foreground(KColorScheme::NormalText).color());
    setDisabledTextColor(colors.scheme.foreground(KColorScheme::InactiveText).color());
    setHighlightedTextColor(colors.selectionScheme.foreground(KColorScheme::NormalText).color());
    setActiveTextColor(colors.scheme.foreground(KColorScheme::ActiveText).color());
    setLinkColor(colors.scheme.foreground(KColorScheme::LinkText).color());
    setVisitedLinkColor(colors.scheme.foreground(KColorScheme::VisitedText).color());
    setNegativeTextColor(colors.scheme.foreground(KColorScheme::NegativeText).color());
    setNeutralTextColor(colors.scheme.foreground(KColorScheme::NeutralText).color());
    setPositiveTextColor(colors.scheme.foreground(KColorScheme::PositiveText).color());


    //background
    setBackgroundColor(colors.scheme.background(KColorScheme::NormalBackground).color());
    setAlternateBackgroundColor(colors.scheme.background(KColorScheme::AlternateBackground).color());
    setHighlightColor(colors.selectionScheme.background(KColorScheme::NormalBackground).color());
    setActiveBackgroundColor(colors.scheme.background(KColorScheme::ActiveBackground).color());
    setLinkBackgroundColor(colors.scheme.background(KColorScheme::LinkBackground).color());
    setVisitedLinkBackgroundColor(colors.scheme.background(KColorScheme::VisitedBackground).color());
    setNegativeBackgroundColor(colors.scheme.background(KColorScheme::NegativeBackground).color());
    setNeutralBackgroundColor(colors.scheme.background(KColorScheme::NeutralBackground).color());
    setPositiveBackgroundColor(colors.scheme.background(KColorScheme::PositiveBackground).color());

    //decoration
    setHoverColor(colors.scheme.decoration(KColorScheme::HoverColor).color());
    setFocusColor(colors.scheme.decoration(KColorScheme::FocusColor).color());

    //legacy stuff
    m_buttonTextColor = (*s_style)->buttonScheme.foreground(KColorScheme::NormalText).color();
    m_buttonBackgroundColor = (*s_style)->buttonScheme.background(KColorScheme::NormalBackground).color();
    m_buttonHoverColor = (*s_style)->buttonScheme.decoration(KColorScheme::HoverColor).color();
    m_buttonFocusColor = (*s_style)->buttonScheme.decoration(KColorScheme::FocusColor).color();

    m_viewTextColor = (*s_style)->viewScheme.foreground(KColorScheme::NormalText).color();
    m_viewBackgroundColor = (*s_style)->viewScheme.background(KColorScheme::NormalBackground).color();
    m_viewHoverColor = (*s_style)->viewScheme.decoration(KColorScheme::HoverColor).color();
    m_viewFocusColor = (*s_style)->viewScheme.decoration(KColorScheme::FocusColor).color();

    // Breeze QQC2 style colors
    auto separatorColor = [](const QColor &bg, const QColor &fg, const qreal baseRatio = 0.2) {
        return KColorUtils::luma(bg) > 0.5 ? KColorUtils::mix(bg, fg, baseRatio) : KColorUtils::mix(bg, fg, baseRatio/2);
    };

    m_buttonSeparatorColor = separatorColor(m_buttonBackgroundColor, m_buttonTextColor, 0.3);

    switch (colorSet()) {
//     case ColorSet::View:
//     case ColorSet::Window:
    case ColorSet::Button:
        m_separatorColor = m_buttonSeparatorColor;
        break;
    case ColorSet::Selection:
        m_separatorColor = focusColor();
        break;
//     case ColorSet::Tooltip:
//     case ColorSet::Complementary:
//     case ColorSet::Header:
    default:
        m_separatorColor = separatorColor(backgroundColor(), textColor());
    }

    emit colorsChanged();
}

QColor PlasmaDesktopTheme::buttonTextColor() const
{
    qWarning() << "WARNING: buttonTextColor is deprecated, use textColor with colorSet: Theme.Button instead";
    return m_buttonTextColor;
}

QColor PlasmaDesktopTheme::buttonBackgroundColor() const
{
    qWarning() << "WARNING: buttonBackgroundColor is deprecated, use backgroundColor with colorSet: Theme.Button instead";
    return m_buttonBackgroundColor;
}

QColor PlasmaDesktopTheme::buttonHoverColor() const
{
    qWarning() << "WARNING: buttonHoverColor is deprecated, use backgroundColor with colorSet: Theme.Button instead";
    return m_buttonHoverColor;
}

QColor PlasmaDesktopTheme::buttonFocusColor() const
{
    qWarning() << "WARNING: buttonFocusColor is deprecated, use backgroundColor with colorSet: Theme.Button instead";
    return m_buttonFocusColor;
}


QColor PlasmaDesktopTheme::viewTextColor() const
{
    qWarning()<<"WARNING: viewTextColor is deprecated, use backgroundColor with colorSet: Theme.View instead";
    return m_viewTextColor;
}

QColor PlasmaDesktopTheme::viewBackgroundColor() const
{
    qWarning() << "WARNING: viewBackgroundColor is deprecated, use backgroundColor with colorSet: Theme.View instead";
    return m_viewBackgroundColor;
}

QColor PlasmaDesktopTheme::viewHoverColor() const
{
    qWarning() << "WARNING: viewHoverColor is deprecated, use backgroundColor with colorSet: Theme.View instead";
    return m_viewHoverColor;
}

QColor PlasmaDesktopTheme::viewFocusColor() const
{
    qWarning() << "WARNING: viewFocusColor is deprecated, use backgroundColor with colorSet: Theme.View instead";
    return m_viewFocusColor;
}

// Breeze QQC2 style colors
QColor PlasmaDesktopTheme::separatorColor() const
{
    return m_separatorColor;
}

QColor PlasmaDesktopTheme::buttonSeparatorColor() const
{
    return m_buttonSeparatorColor;
}

#include "plasmadesktoptheme.moc"
