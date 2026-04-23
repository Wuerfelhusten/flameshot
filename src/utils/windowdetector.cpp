// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 Flameshot Contributors

#include "windowdetector.h"

#if defined(Q_OS_WIN)

#include <QCursor>
#include <QGuiApplication>
#include <QScreen>
#include <windows.h>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/**
 * Convert a Win32 physical-pixel RECT to a QRect expressed in Qt logical
 * pixel coordinates.
 *
 * physCursor is the physical cursor position (from GetCursorPos) which lies
 * within the found window.  We use the simultaneous logical cursor position
 * from QCursor::pos() to derive the per-monitor physical origin without any
 * assumptions about the monitor layout.  This correctly handles monitors
 * placed to the left/above the primary and mixed-DPI configurations.
 */
static QRect physicalToLogicalRect(const RECT& physRect, POINT physCursor)
{
    // QCursor::pos() returns the same point in logical (DPI-independent) coords.
    const QPoint logCursor = QCursor::pos();

    // Find the Qt screen containing the logical cursor position.
    QScreen* screen = QGuiApplication::screenAt(logCursor);
    if (!screen)
        screen = QGuiApplication::primaryScreen();

    if (!screen) {
        // Last-resort: identity mapping
        return QRect(physRect.left,
                     physRect.top,
                     physRect.right  - physRect.left,
                     physRect.bottom - physRect.top);
    }

    const QRect logGeom = screen->geometry();
    const qreal dpr     = screen->devicePixelRatio();

    // Derive the Win32 physical origin of this monitor from the two
    // representations of the cursor position:
    //   physCursor = physMonOrigin + (logCursor - logGeom.topLeft()) * dpr
    const qreal physMonOriginX =
        physCursor.x - (logCursor.x() - logGeom.x()) * dpr;
    const qreal physMonOriginY =
        physCursor.y - (logCursor.y() - logGeom.y()) * dpr;

    return QRect(
        logGeom.x() + qRound((physRect.left - physMonOriginX) / dpr),
        logGeom.y() + qRound((physRect.top  - physMonOriginY) / dpr),
        qRound((physRect.right  - physRect.left) / dpr),
        qRound((physRect.bottom - physRect.top)  / dpr));
}

// ---------------------------------------------------------------------------
// EnumWindows callback
// ---------------------------------------------------------------------------

struct FindWindowData
{
    POINT pt;         ///< Physical cursor position to test.
    HWND  skipHwnd;   ///< Window to skip (the caller's own window).
    HWND  result;     ///< Set to the found window handle, or NULL.
};

static BOOL CALLBACK findTopLevelWindowAtPoint(HWND hwnd, LPARAM lParam)
{
    auto* data = reinterpret_cast<FindWindowData*>(lParam);

    // Skip the window we want to ignore (usually the Flameshot overlay).
    if (hwnd == data->skipHwnd)
        return TRUE;

    // Skip invisible and minimised windows.
    if (!IsWindowVisible(hwnd) || IsIconic(hwnd))
        return TRUE;

    // Skip tool-windows and popup menus that should not be selectable.
    const LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    if (exStyle & WS_EX_TOOLWINDOW)
        return TRUE;

    RECT r{};
    if (!GetWindowRect(hwnd, &r))
        return TRUE;

    if (PtInRect(&r, data->pt)) {
        data->result = hwnd;
        return FALSE; // stop enumeration – we found our window
    }

    return TRUE; // continue
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

WindowInfo WindowDetector::windowAt(QPoint /*globalLogicalPos*/,
                                    quintptr skipNativeId)
{
    WindowInfo result;

    // Use the physical cursor position for Win32 APIs.
    POINT physPt{};
    if (!GetCursorPos(&physPt))
        return result;

    FindWindowData data{};
    data.pt = physPt;
    data.skipHwnd = reinterpret_cast<HWND>(skipNativeId);
    data.result = nullptr;

    EnumWindows(findTopLevelWindowAtPoint, reinterpret_cast<LPARAM>(&data));

    HWND hwnd = data.result;
    if (!hwnd)
        return result;

    RECT physRect{};
    if (!GetWindowRect(hwnd, &physRect))
        return result;

    result.geometry = physicalToLogicalRect(physRect, physPt);
    result.valid = true;
    return result;
}

#else // non-Windows stub

WindowInfo WindowDetector::windowAt(QPoint, quintptr)
{
    return {};
}

#endif // Q_OS_WIN
