// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 Flameshot Contributors

#pragma once

#include <QPoint>
#include <QRect>

/// Describes the result of a window-at-cursor look-up.
struct WindowInfo
{
    QRect geometry; ///< Window geometry in Qt logical pixel coordinates.
    bool valid = false;
};

/**
 * @brief Platform utility for finding the native window under the cursor.
 *
 * Currently only implemented on Windows. Other platforms always return an
 * invalid WindowInfo.
 */
class WindowDetector
{
public:
    /**
     * @brief Return the top-level native window that contains @p
     * globalLogicalPos.
     *
     * @param globalLogicalPos  Cursor position in Qt global logical
     * coordinates.
     * @param skipNativeId      Native window handle (HWND on Windows) to
     * exclude from the search (typically the caller's own window). Pass 0 to
     * skip no window.
     * @return WindowInfo with valid=true and the window geometry in Qt logical
     *         coordinates, or valid=false if no suitable window was found.
     */
    static WindowInfo windowAt(QPoint globalLogicalPos,
                               quintptr skipNativeId = 0);
};
