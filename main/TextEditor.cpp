#include <algorithm>
#include <chrono>
#include <string>
#define NOMINMAX
#include "TextEditor.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h" // for imGui::GetCurrentWindow()

// ----------------------------------------------
//
// ----------------------------------------------
template<class InputIt1, class InputIt2, class BinaryPredicate>
bool equals(InputIt1 first1, InputIt1 last1,
    InputIt2 first2, InputIt2 last2, BinaryPredicate p)
{
    for (; first1 != last1 && first2 != last2; ++first1, ++first2)
    {
        if (!p(*first1, *first2))
            return false;
    }
    return first1 == last1 && first2 == last2;
}
// ----------------------------------------------
//
// ----------------------------------------------
TextEditor::TextEditor()
    : mLineSpacing(1.0f)
    , mUndoIndex(0)
    , mTabSize(4)
    , mOverwrite(false)
    , mReadOnly(false)
    , mWithinRender(false)
    , mScrollToCursor(false)
    , mScrollToTop(false)
    , mTextChanged(false)
    , mTextStart(20.0f)
    , mLeftMargin(10)
    , mColorRangeMin(0)
    , mColorRangeMax(0)
    , mSelectionMode(SelectionMode::Normal)
    , mLastClick(-1.0f)
{
    mLines.push_back(Line());
}
// ----------------------------------------------
//
// ----------------------------------------------
TextEditor::~TextEditor()
{
}
// ----------------------------------------------
//
// ----------------------------------------------
int TextEditor::AppendBuffer(CText& aBuffer, CText::Char chr, int aIndex)
{
    if (chr != _T('\t'))
    {
        aBuffer.push_back(chr);
        return aIndex + 1;
    }
}
// ----------------------------------------------
//
// ----------------------------------------------
CText TextEditor::GetText(const Coordinates& aStart, const Coordinates& aEnd) const
{
    CText result;

    int prevLineNo = aStart.mLine;
    for (auto it = aStart; it <= aEnd; Advance(it))
    {
        if (prevLineNo != it.mLine && it.mLine < (int)mLines.size())
        {
            result.push_back(_T('\n'));
        }

        if (it == aEnd)
        {
            break;
        }

        prevLineNo = it.mLine;
        const auto& line = mLines[it.mLine];
        if (!line.empty() && it.mColumn < (int)line.size())
        {
            result.push_back(line[it.mColumn].mChar);
        }
    }
    return result;
}
// ----------------------------------------------
//
// ----------------------------------------------
TextEditor::Coordinates TextEditor::GetActualCursorCoordinates() const
{
    return SanitizeCoordinates(mState.mCursorPosition);
}
// ----------------------------------------------
//
// ----------------------------------------------
TextEditor::Coordinates TextEditor::SanitizeCoordinates(const Coordinates& aValue) const
{
    auto line = aValue.mLine;
    auto column = aValue.mColumn;

    if (line >= (int)mLines.size())
    {
        if (mLines.empty())
        {
            line = 0;
            column = 0;
        }
        else
        {
            line = (int)mLines.size() - 1;
            column = (int)mLines[line].size();
        }
    }
    else
    {
        column = mLines.empty() ? 0 : std::min((int)mLines[line].size(), aValue.mColumn);
    }
    return Coordinates(line, column);
}
// ----------------------------------------------
//
// ----------------------------------------------
void TextEditor::Advance(Coordinates& aCoordinates) const
{
    if (aCoordinates.mLine < (int)mLines.size())
    {
        auto& line = mLines[aCoordinates.mLine];
        if (aCoordinates.mColumn + 1 < (int)line.size())
        {
            ++aCoordinates.mColumn;
        }
        else
        {
            ++aCoordinates.mLine;
            aCoordinates.mColumn = 0;
        }
    }
}
// ----------------------------------------------
//
// ----------------------------------------------
void TextEditor::DeleteRange(const Coordinates& aStart, const Coordinates& aEnd)
{
    assert(aEnd >= aStart);
    assert(!mReadOnly);

    if (aEnd == aStart)
    {
        return;
    }

    if (aStart.mLine == aEnd.mLine)
    {
        auto& line = mLines[aStart.mLine];
        if (aEnd.mColumn >= (int)line.size())
        {
            line.erase(line.begin() + aStart.mColumn, line.end());
        }
        else
        {
            line.erase(line.begin() + aStart.mColumn, line.begin() + aEnd.mColumn);
        }
    }
    else
    {
        auto& firstLine = mLines[aStart.mLine];
        auto& lastLine = mLines[aEnd.mLine];

        firstLine.erase(firstLine.begin() + aStart.mColumn, firstLine.end());
        lastLine.erase(lastLine.begin(), lastLine.begin() + aEnd.mColumn);

        if (aStart.mLine < aEnd.mLine)
        {
            firstLine.insert(firstLine.end(), lastLine.begin(), lastLine.end());
        }
        if (aStart.mLine < aEnd.mLine)
        {
            RemoveLine(aStart.mLine + 1, aEnd.mLine + 1);
        }
    }
    mTextChanged = true;
}
// ----------------------------------------------
//
// ----------------------------------------------
int TextEditor::InsertTextAt(Coordinates& /* inout */ aWhere, const CText::Char* aValue)
{
    assert(!mReadOnly);

    int totalLines = 0;
    auto chr = *aValue;
    while (chr != _T('\0'))
    {
        assert(!mLines.empty());
        if (chr == _T('\r'))
        {
            // skip
        }
        else if (chr == _T('\n'))
        {
            if (aWhere.mColumn < (int)mLines[aWhere.mLine].size())
            {
                auto& newLine = InsertLine(aWhere.mLine + 1);
                auto& line = mLines[aWhere.mLine];
                newLine.insert(newLine.begin(), line.begin() + aWhere.mColumn, line.end());
                line.erase(line.begin() + aWhere.mColumn, line.end());
            }
            else
            {
                InsertLine(aWhere.mLine + 1);
            }
            ++aWhere.mLine;
            aWhere.mColumn = 0;
            ++totalLines;
        }
        else
        {
            auto& line = mLines[aWhere.mLine];
            line.insert(line.begin() + aWhere.mColumn, Glyph(chr, ImVec4(255, 0, 255, 255)));
            ++aWhere.mColumn;
        }
        chr = *(++aValue);
        mTextChanged = true;
    }
    return totalLines;
}
// ----------------------------------------------
//
// ----------------------------------------------
void TextEditor::AddUndo(UndoRecord& aValue)
{
    assert(!mReadOnly);
    mUndoBuffer.resize(mUndoIndex + 1);
    mUndoBuffer.back() = aValue;
    ++mUndoIndex;
}
// ----------------------------------------------
//
// ----------------------------------------------
TextEditor::Coordinates TextEditor::ScreenPosToCoordinates(const ImVec2& aPosition) const
{
    ImVec2 origin = ImGui::GetCursorScreenPos();
    ImVec2 local(aPosition.x - origin.x, aPosition.y - origin.y);
    int lineNo = std::max(0, (int)floor(local.y / mCharAdvance.y));
    // ------------------------------------------
    // Compute columnCoord according to text size
    // ------------------------------------------ 
    int columnCoord = 0;
    float columnWidth = 0.0f;
    CText cumulatedString = _T("");
    float cumulatedStringWidth[2] = { 0.0f, 0.0f }; //( [0] is the lastest, [1] is the previous. I use that trick to check where cursor is exactly (important for tabs)

    if (lineNo >= 0 && lineNo < (int)mLines.size())
    {
        auto& line = mLines.at(lineNo);
        // First we find the hovered column coord.
        while (mTextStart + cumulatedStringWidth[0] < local.x &&
            (size_t)columnCoord < line.size())
        {
            cumulatedStringWidth[1] = cumulatedStringWidth[0];
            cumulatedString += line[columnCoord].mChar;
            std::string b = cumulatedString.toSingle().str();
            cumulatedStringWidth[0] = font->CalcTextSizeA(font->FontSize, FLT_MAX, -1.0f, b.c_str(), nullptr, nullptr).x;
            columnWidth = (cumulatedStringWidth[0] - cumulatedStringWidth[1]);
            columnCoord++;
        }
        // Then we reduce by 1 column coord if cursor is on the left side of the hovered column.
        if (mTextStart + cumulatedStringWidth[0] - columnWidth / 2.0f > local.x)
        {
            columnCoord = std::max(0, columnCoord - 1);
        }
    }
    return SanitizeCoordinates(Coordinates(lineNo, columnCoord));
}
// ----------------------------------------------
//
// ----------------------------------------------
TextEditor::Coordinates TextEditor::FindWordStart(const Coordinates& aFrom) const
{
    Coordinates at = aFrom;
    if (at.mLine >= (int)mLines.size())
    {
        return at;
    }
    auto& line = mLines[at.mLine];

    if (at.mColumn >= (int)line.size())
    {
        return at;
    }

    if (iswspace(line[at.mColumn].mChar))
    {
        return at;
    }
    //auto cstart = (PaletteIndex)line[at.mColumn].mColorIndex;
    while (at.mColumn > 0)
    {
        if (iswspace(line[at.mColumn].mChar))
        {
            ++at.mColumn;
            break;
        }
        --at.mColumn;
    }
    return at;
}
// ----------------------------------------------
//
// ----------------------------------------------
TextEditor::Coordinates TextEditor::FindWordEnd(const Coordinates& aFrom) const
{
    Coordinates at = aFrom;
    if (at.mLine >= (int)mLines.size())
    {
        return at;
    }
    auto& line = mLines[at.mLine];

    if (at.mColumn >= (int)line.size())
    {
        return at;
    }

    if (iswspace(line[at.mColumn].mChar))
    {
        return at;
    }

    while (at.mColumn < (int)line.size())
    {
        if (iswspace(line[at.mColumn].mChar))
        {
            break;
        }
        ++at.mColumn;
    }
    return at;
}
// ----------------------------------------------
//
// ----------------------------------------------
void TextEditor::RemoveLine(int aStart, int aEnd)
{
    assert(!mReadOnly);
    assert(aEnd >= aStart);
    assert(mLines.size() > (size_t)(aEnd - aStart));
    mLines.erase(mLines.begin() + aStart, mLines.begin() + aEnd);
    assert(!mLines.empty());
    mTextChanged = true;
}
// ----------------------------------------------
//
// ----------------------------------------------
void TextEditor::RemoveLine(int aIndex)
{
    assert(!mReadOnly);
    assert(mLines.size() > 1);
    mLines.erase(mLines.begin() + aIndex);
    assert(!mLines.empty());
    mTextChanged = true;
}
// ----------------------------------------------
//
// ----------------------------------------------
TextEditor::Line& TextEditor::InsertLine(int aIndex)
{
    assert(!mReadOnly);
    auto& result = *mLines.insert(mLines.begin() + aIndex, Line());
    return result;
}
// ----------------------------------------------
//
// ----------------------------------------------
CText TextEditor::GetWordUnderCursor() const
{
    auto c = GetCursorPosition();
    return GetWordAt(c);
}
// ----------------------------------------------
//
// ----------------------------------------------
CText TextEditor::GetWordAt(const Coordinates& aCoords) const
{
    auto start = FindWordStart(aCoords);
    auto end = FindWordEnd(aCoords);
    CText r;
    for (auto it = start; it < end; Advance(it))
    {
        r.push_back(mLines[it.mLine][it.mColumn].mChar);
    }
    return r;
}
// ----------------------------------------------
//
// ----------------------------------------------
ImU32 TextEditor::GetGlyphColor(const Glyph& aGlyph) const
{
    return ImGui::ColorConvertFloat4ToU32(aGlyph.mColor);
}
// ----------------------------------------------
//
// ----------------------------------------------
void TextEditor::HandleKeyboardInputs()
{
    ImGuiIO& io = ImGui::GetIO();
    auto shift = io.KeyShift;
    auto ctrl = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;
    auto alt = io.ConfigMacOSXBehaviors ? io.KeyCtrl : io.KeyAlt;

    if (ImGui::IsWindowFocused())
    {
        if (ImGui::IsWindowHovered())
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_TextInput);
        }

        io.WantCaptureKeyboard = true;
        io.WantTextInput = true;

        if (!IsReadOnly() && ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Z)))
            Undo();
        else if (!IsReadOnly() && !ctrl && !shift && alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Backspace)))
            Undo();
        else if (!IsReadOnly() && ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Y)))
            Redo();
        else if (!ctrl && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_UpArrow)))
            MoveUp(1, shift);
        else if (!ctrl && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_DownArrow)))
            MoveDown(1, shift);
        else if (!alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_LeftArrow)))
            MoveLeft(1, shift, ctrl);
        else if (!alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_RightArrow)))
            MoveRight(1, shift, ctrl);
        else if (!alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_PageUp)))
            MoveUp(GetPageSize() - 4, shift);
        else if (!alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_PageDown)))
            MoveDown(GetPageSize() - 4, shift);
        else if (!alt && ctrl && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Home)))
            MoveTop(shift);
        else if (ctrl && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_End)))
            MoveBottom(shift);
        else if (!ctrl && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Home)))
            MoveHome(shift);
        else if (!ctrl && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_End)))
            MoveEnd(shift);
        else if (!IsReadOnly() && !ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Delete)))
            Delete();
        else if (!IsReadOnly() && !ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Backspace)))
            BackSpace();
        else if (!ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Insert)))
            mOverwrite ^= true;
        else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Insert)))
            Copy();
        else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_C)))
            Copy();
        else if (!IsReadOnly() && !ctrl && shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Insert)))
            Paste();
        else if (!IsReadOnly() && ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_V)))
            Paste();
        else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_X)))
            Cut();
        else if (!ctrl && shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Delete)))
            Cut();
        else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_A)))
            SelectAll();
        else if (!IsReadOnly() && !ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter)))
            EnterCharacter(_T('\n'), false);
        else if (!IsReadOnly() && !ctrl && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Tab)))
            EnterCharacter(_T('\t'), shift);
        else if (!IsReadOnly() && !ctrl && !alt)
        {
            for (int i = 0; i < io.InputQueueCharacters.Size; i++)
            {
                auto c = io.InputQueueCharacters[i];
                if (c != 0)
                {
                    if (iswprint(c) || iswspace(c))
                    {
                        EnterCharacter(c, shift);
                    }
                }
                //std::wcout << (Char)c << std::endl;
            }
            io.InputQueueCharacters.resize(0);
        }
    }
}
// ----------------------------------------------
//
// ----------------------------------------------
void TextEditor::HandleMouseInputs()
{
    ImGuiIO& io = ImGui::GetIO();
    auto shift = io.KeyShift;
    auto ctrl = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;
    auto alt = io.ConfigMacOSXBehaviors ? io.KeyCtrl : io.KeyAlt;

    if (ImGui::IsWindowHovered())
    {
        if (!shift && !alt)
        {
            auto click = ImGui::IsMouseClicked(0);
            auto doubleClick = ImGui::IsMouseDoubleClicked(0);
            auto t = ImGui::GetTime();
            auto tripleClick = click && !doubleClick && (mLastClick != -1.0f && (t - mLastClick) < io.MouseDoubleClickTime);

            // ------------------------------
            // Left mouse button triple click
            // -------------------------------

            if (tripleClick)
            {
                if (!ctrl)
                {
                    mState.mCursorPosition = mInteractiveStart = mInteractiveEnd = SanitizeCoordinates(ScreenPosToCoordinates(ImGui::GetMousePos()));
                    mSelectionMode = SelectionMode::Line;
                    SetSelection(mInteractiveStart, mInteractiveEnd, mSelectionMode);
                }

                mLastClick = -1.0f;
            }

            // -------------------------------
            // Left mouse button double click
            // --------------------------------

            else if (doubleClick)
            {
                if (!ctrl)
                {
                    mState.mCursorPosition = mInteractiveStart = mInteractiveEnd = SanitizeCoordinates(ScreenPosToCoordinates(ImGui::GetMousePos()));
                    if (mSelectionMode == SelectionMode::Line)
                        mSelectionMode = SelectionMode::Normal;
                    else
                        mSelectionMode = SelectionMode::Word;
                    SetSelection(mInteractiveStart, mInteractiveEnd, mSelectionMode);
                }

                mLastClick = (float)ImGui::GetTime();
            }

            // -----------------------
            // Left mouse button click
            // ------------------------
            else if (click)
            {
                mState.mCursorPosition = mInteractiveStart = mInteractiveEnd = SanitizeCoordinates(ScreenPosToCoordinates(ImGui::GetMousePos()));
                if (ctrl)
                    mSelectionMode = SelectionMode::Word;
                else
                    mSelectionMode = SelectionMode::Normal;
                SetSelection(mInteractiveStart, mInteractiveEnd, mSelectionMode);

                mLastClick = (float)ImGui::GetTime();
            }
            // Mouse left button dragging (=> update selection)
            else if (ImGui::IsMouseDragging(0) && ImGui::IsMouseDown(0))
            {
                io.WantCaptureMouse = true;
                mState.mCursorPosition = mInteractiveEnd = SanitizeCoordinates(ScreenPosToCoordinates(ImGui::GetMousePos()));
                SetSelection(mInteractiveStart, mInteractiveEnd, mSelectionMode);
            }
        }
    }
}
// ----------------------------------------------
//
// ----------------------------------------------
void TextEditor::Render()
{
    std::string b;
    /* Compute mCharAdvance regarding to scaled font size (Ctrl + mouse wheel)*/
    //ImGuiIO& io = ImGui::GetIO();
    ImGuiStyle& style = ImGui::GetStyle();
    ImGui::PushFont(font);

    const float fontSize = font->CalcTextSizeA(font->FontSize, FLT_MAX, -1.0f, (char*)"#", nullptr, nullptr).x;
    mCharAdvance = ImVec2(fontSize, ImGui::GetTextLineHeightWithSpacing() * mLineSpacing);


    static CText buffer;
    assert(buffer.isEmpty());

    auto contentSize = ImGui::GetWindowContentRegionMax();
    auto drawList = ImGui::GetWindowDrawList();
    float longest(mTextStart);

    if (mScrollToTop)
    {
        mScrollToTop = false;
        ImGui::SetScrollY(0.f);
    }

    ImVec2 cursorScreenPos = ImGui::GetCursorScreenPos();
    auto scrollX = ImGui::GetScrollX();
    auto scrollY = ImGui::GetScrollY();

    auto lineNo = (int)floor(scrollY / mCharAdvance.y);
    auto globalLineMax = (int)mLines.size();
    auto lineMax = std::max(0, std::min((int)mLines.size() - 1, lineNo + (int)floor((scrollY + contentSize.y) / mCharAdvance.y)));

    // Deduce mTextStart by evaluating mLines size (global lineMax) plus two spaces as text width
    CText::Char buf[16];
    _snwprintf(buf, 16, _T(" %d "), globalLineMax);
    b = CText(buf).toSingle().str();
    mTextStart = font->CalcTextSizeA(font->FontSize, FLT_MAX, -1.0f, (char*)b.c_str(), nullptr, nullptr).x + mLeftMargin;

    if (!mLines.empty())
    {
        auto fontScale = 1;// ImGui::GetFontSize() / font->FontSize;
        float spaceSize = font->CalcTextSizeA(font->FontSize, FLT_MAX, -1.0f, (char*)" ", nullptr, nullptr).x;

        while (lineNo <= lineMax)
        {
            ImVec2 lineStartScreenPos = ImVec2(cursorScreenPos.x, cursorScreenPos.y + lineNo * mCharAdvance.y);
            ImVec2 textScreenPos = ImVec2(lineStartScreenPos.x + mTextStart, lineStartScreenPos.y);

            auto& line = mLines[lineNo];
            longest = std::max(mTextStart + TextDistanceToLineStart(Coordinates(lineNo, (int)line.size())), longest);
            auto columnNo = 0;
            Coordinates lineStartCoord(lineNo, 0);
            Coordinates lineEndCoord(lineNo, (int)line.size());

            // Draw selection for the current line
            float sstart = -1.0f;
            float ssend = -1.0f;

            assert(mState.mSelectionStart <= mState.mSelectionEnd);
            if (mState.mSelectionStart <= lineEndCoord)
                sstart = mState.mSelectionStart > lineStartCoord ? TextDistanceToLineStart(mState.mSelectionStart) : 0.0f;
            if (mState.mSelectionEnd > lineStartCoord)
                ssend = TextDistanceToLineStart(mState.mSelectionEnd < lineEndCoord ? mState.mSelectionEnd : lineEndCoord);

            if (mState.mSelectionEnd.mLine > lineNo)
                ssend += mCharAdvance.x;

            if (sstart != -1 && ssend != -1 && sstart < ssend)
            {
                ImVec2 vstart(lineStartScreenPos.x + mTextStart + sstart, lineStartScreenPos.y);
                ImVec2 vend(lineStartScreenPos.x + mTextStart + ssend, lineStartScreenPos.y + mCharAdvance.y);
                drawList->AddRectFilled(vstart, vend, ImGui::ColorConvertFloat4ToU32(ImVec4(0.2, 0.2, 0.2, 1))); // фон выделения
            }

            // Draw breakpoints
            auto start = ImVec2(lineStartScreenPos.x + scrollX, lineStartScreenPos.y);
            // Highlight the current line (where the cursor is)
            if (mState.mCursorPosition.mLine == lineNo)
            {
                auto focused = ImGui::IsWindowFocused();

                if (!HasSelection())
                {
                    auto end = ImVec2(start.x + contentSize.x + scrollX, start.y + mCharAdvance.y);
                    drawList->AddRectFilled(start, end, focused ? ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0.1, 0.1, 1)) : ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0.1, 0.1, 1)));
                    drawList->AddRect(start, end, ImGui::ColorConvertFloat4ToU32(ImVec4(0.5, 0.5, 0.5, 1)), 1.0f);
                }

                float cx = TextDistanceToLineStart(mState.mCursorPosition);

                // курсор 
                if (focused)
                {
                    static auto timeStart = std::chrono::system_clock::now();
                    auto timeEnd = std::chrono::system_clock::now();
                    auto diff = timeEnd - timeStart;
                    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(diff).count();
                    if (elapsed > 400)
                    {
                        ImVec2 cstart(textScreenPos.x + cx, lineStartScreenPos.y);
                        ImVec2 cend(textScreenPos.x + cx + (mOverwrite ? mCharAdvance.x : 1.0f), lineStartScreenPos.y + mCharAdvance.y);
                        drawList->AddRectFilled(cstart, cend, ImGui::ColorConvertFloat4ToU32(ImVec4(0.2, 1.0, 0.2, 1)));
                        if (elapsed > 800)
                        {
                            timeStart = timeEnd;
                        }
                    }
                }
            }

            // Render colorized text
            auto prevColor = line.empty() ? ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 255, 255)) : GetGlyphColor(line[0]);
            ImVec2 bufferOffset;

            for (auto& glyph : line)
            {
                auto color = GetGlyphColor(glyph); // foreground color

                if ((color != prevColor || glyph.mChar == _T('\t')) && !buffer.isEmpty())
                {
                    const ImVec2 newOffset(textScreenPos.x + bufferOffset.x, textScreenPos.y + bufferOffset.y);
                    b = buffer.toSingle().str();
                    drawList->AddText(newOffset, prevColor, (char*)b.c_str());
                    auto textSize = font->CalcTextSizeA(font->FontSize, FLT_MAX, -1.0f, (char*)b.c_str(), nullptr, nullptr);
                    bufferOffset.x += textSize.x;
                    buffer.clear();
                }
                prevColor = color;

                if (glyph.mChar == _T('\t'))
                {
                    bufferOffset.x = (1.0f * fontScale + std::floor((1.0f + bufferOffset.x)) / (float(mTabSize) * spaceSize)) * (float(mTabSize) * spaceSize);
                }
                else
                {
                    AppendBuffer(buffer, glyph.mChar, 0);
                }
                ++columnNo;
            }

            if (!buffer.isEmpty())
            {
                const ImVec2 newOffset(textScreenPos.x + bufferOffset.x, textScreenPos.y + bufferOffset.y);
                b = buffer.toSingle().str();
                drawList->AddText(newOffset, prevColor, (char*)b.c_str());
                buffer.clear();
            }
            // draw line numbers
            ImU32 Col = ImGui::ColorConvertFloat4ToU32(ImVec4(1, 1, 1, 1));
            _snwprintf(buf, 16, _T(" %d  "), lineNo + 1);
            b = CText(buf).toSingle().str();
            auto lineNoWidth = font->CalcTextSizeA(font->FontSize, FLT_MAX, -1.0f, (char*)b.c_str(), nullptr, nullptr).x;
            drawList->AddText(ImVec2(lineStartScreenPos.x + mTextStart - lineNoWidth, lineStartScreenPos.y), Col, (char*)b.c_str());

            lineNo++;

        }

        // Draw a tooltip on known identifiers/preprocessor symbols
        if (ImGui::IsMousePosValid())
        {
            auto id = GetWordAt(ScreenPosToCoordinates(ImGui::GetMousePos()));
            if (!id.isEmpty())
            {
                b = id.toSingle().str();
                //wstrtostr(id);
                ImGui::BeginTooltip();
                ImGui::TextUnformatted(b.c_str());
                ImGui::EndTooltip();
            }
        }
    }
    ImGui::Dummy(ImVec2((longest + 2), mLines.size() * mCharAdvance.y));
    if (mScrollToCursor)
    {
        EnsureCursorVisible();
        ImGui::SetWindowFocus();
        mScrollToCursor = false;
    }
    ImGui::PopFont();
}
// ----------------------------------------------
//
// ----------------------------------------------
int colorIndexFromName(std::string col_name)
{
    int ind = -1;
    for (int i = 0; i < ImGuiCol_COUNT; i++)
    {
        char* name = (char*)ImGui::GetStyleColorName(i);
        if (name == col_name)
        {
            ind = i;
            break;
        }
    }
    return ind;
}
// ----------------------------------------------
//
// ----------------------------------------------
void TextEditor::Render(const CText::Char* aTitle, const ImVec2& aSize, bool aBorder)
{
    ImGuiStyle& style = ImGui::GetStyle();
    mWithinRender = true;
    mTextChanged = false;
    mCursorPositionChanged = false;
    ImVec4 bg_col = style.Colors[colorIndexFromName("ChildBg")];
    ImGui::PushStyleColor(3, bg_col); // background
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
    ImGui::BeginChild((char*)aTitle, aSize, aBorder, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_NoMove);
    ImGui::PushAllowKeyboardFocus(true);

    HandleKeyboardInputs();
    HandleMouseInputs();
    Render();

    ImGui::PopAllowKeyboardFocus();
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

    mWithinRender = false;
}
// ----------------------------------------------
//
// ----------------------------------------------
void TextEditor::SetText(const CText& aText)
{
    mLines.clear();
    mLines.emplace_back(Line());
    for (int i=0;i<aText.length();++i)
    {
        auto chr = aText[i];
        if (chr == _T('\r'))
        {
            // ignore the carriage return character
        }
        else if (chr == _T('\n'))
        {
            mLines.emplace_back(Line());
        }
        else
        {
            mLines.back().emplace_back(Glyph(chr, ImVec4(255, 0, 255, 255)));
        }
    }
    mTextChanged = true;
    mScrollToTop = true;
    mUndoBuffer.clear();
    mUndoIndex = 0;
    Colorize(this,mLines);
}
// ----------------------------------------------
//
// ----------------------------------------------
void TextEditor::SetTextLines(const std::vector<CText>& aLines)
{
    mLines.clear();

    if (aLines.empty())
    {
        mLines.emplace_back(Line());
    }
    else
    {
        mLines.resize(aLines.size());

        for (size_t i = 0; i < aLines.size(); ++i)
        {
            const CText& aLine = aLines[i];

            mLines[i].reserve(aLine.length());
            for (size_t j = 0; j < aLine.length(); ++j)
            {
                mLines[i].emplace_back(Glyph(aLine[j], ImVec4(255, 0, 255, 255)));
            }
        }
    }
    mTextChanged = true;
    mScrollToTop = true;
    mUndoBuffer.clear();
    mUndoIndex = 0;
    Colorize(this, mLines);
}
// ----------------------------------------------
//
// ----------------------------------------------
void TextEditor::EnterCharacter(CText::Char aChar, bool aShift)
{
    assert(!mReadOnly);

    UndoRecord u;

    u.mBefore = mState;

    if (HasSelection())
    {
        if (aChar == _T('\t'))
        {
            auto start = mState.mSelectionStart;
            auto end = mState.mSelectionEnd;

            if (start > end)
            {
                std::swap(start, end);
            }
            start.mColumn = 0;
            //			end.mColumn = end.mLine < mLines.size() ? mLines[end.mLine].size() : 0;
            if (end.mColumn == 0 && end.mLine > 0)
            {
                --end.mLine;
                end.mColumn = (int)mLines[end.mLine].size();
            }

            u.mRemovedStart = start;
            u.mRemovedEnd = end;
            u.mRemoved = GetText(start, end);

            bool modified = false;

            for (int i = start.mLine; i <= end.mLine; i++)
            {
                auto& line = mLines[i];
                if (aShift)
                {
                    if (line.empty() == false)
                    {
                        if (line.front().mChar == _T('\t'))
                        {
                            line.erase(line.begin());
                            if (i == end.mLine && end.mColumn > 0)
                            {
                                end.mColumn--;
                            }
                            modified = true;
                        }
                    }
                    else
                    {
                        for (int j = 0; j < mTabSize && line.empty() == false && line.front().mChar == _T(' '); j++)
                        {
                            line.erase(line.begin());
                            if (i == end.mLine && end.mColumn > 0)
                            {
                                end.mColumn--;
                            }
                            modified = true;
                        }
                    }
                }
                else
                {
                    line.insert(line.begin(), Glyph(_T('\t'), ImVec4(255, 0, 0, 0)));
                    if (i == end.mLine)
                    {
                        ++end.mColumn;
                    }
                    modified = true;
                }
            }

            if (modified)
            {
                u.mAddedStart = start;
                u.mAddedEnd = end;
                u.mAdded = GetText(start, end);
                mTextChanged = true;
                AddUndo(u);
                EnsureCursorVisible();
            }
            return;
        }
        else
        {
            u.mRemoved = GetSelectedText();
            u.mRemovedStart = mState.mSelectionStart;
            u.mRemovedEnd = mState.mSelectionEnd;
            DeleteSelection();
        }
    }

    auto coord = GetActualCursorCoordinates();
    u.mAddedStart = coord;

    assert(!mLines.empty());

    if (aChar == _T('\n'))
    {
        InsertLine(coord.mLine + 1);
        auto& line = mLines[coord.mLine];
        auto& newLine = mLines[coord.mLine + 1];
        const size_t whitespaceSize = newLine.size();
        newLine.insert(newLine.end(), line.begin() + coord.mColumn, line.end());
        line.erase(line.begin() + coord.mColumn, line.begin() + line.size());
        SetCursorPosition(Coordinates(coord.mLine + 1, (int)whitespaceSize));
    }
    else
    {
        auto& line = mLines[coord.mLine];
        if (mOverwrite && (int)line.size() > coord.mColumn)
        {
            line[coord.mColumn] = Glyph(aChar, ImVec4(255, 0, 255, 255));
        }
        else
        {
            line.insert(line.begin() + coord.mColumn, Glyph(aChar, ImVec4(255, 0, 255, 255)));
        }
        SetCursorPosition(Coordinates(coord.mLine, coord.mColumn + 1));
    }
    mTextChanged = true;
    u.mAdded = aChar;
    u.mAddedEnd = GetActualCursorCoordinates();
    u.mAfter = mState;
    AddUndo(u);
    Colorize(this, mLines);
    EnsureCursorVisible();
}
// ----------------------------------------------
//
// ----------------------------------------------
void TextEditor::SetReadOnly(bool aValue)
{
    mReadOnly = aValue;
}
// ----------------------------------------------
//
// ----------------------------------------------
void TextEditor::SetCursorPosition(const Coordinates& aPosition)
{
    if (mState.mCursorPosition != aPosition)
    {
        mState.mCursorPosition = aPosition;
        mCursorPositionChanged = true;
        EnsureCursorVisible();
    }
}
// ----------------------------------------------
//
// ----------------------------------------------
void TextEditor::SetSelectionStart(const Coordinates& aPosition)
{
    mState.mSelectionStart = SanitizeCoordinates(aPosition);
    if (mState.mSelectionStart > mState.mSelectionEnd)
        std::swap(mState.mSelectionStart, mState.mSelectionEnd);
}
// ----------------------------------------------
//
// ----------------------------------------------
void TextEditor::SetSelectionEnd(const Coordinates& aPosition)
{
    mState.mSelectionEnd = SanitizeCoordinates(aPosition);
    if (mState.mSelectionStart > mState.mSelectionEnd)
        std::swap(mState.mSelectionStart, mState.mSelectionEnd);
}
// ----------------------------------------------
//
// ----------------------------------------------
void TextEditor::SetSelection(const Coordinates& aStart, const Coordinates& aEnd, SelectionMode aMode)
{
    auto oldSelStart = mState.mSelectionStart;
    auto oldSelEnd = mState.mSelectionEnd;

    mState.mSelectionStart = SanitizeCoordinates(aStart);
    mState.mSelectionEnd = SanitizeCoordinates(aEnd);
    if (aStart > aEnd)
    {
        std::swap(mState.mSelectionStart, mState.mSelectionEnd);
    }
    switch (aMode)
    {
    case TextEditor::SelectionMode::Normal:
        break;
    case TextEditor::SelectionMode::Word:
    {
        mState.mSelectionStart = FindWordStart(mState.mSelectionStart);
        mState.mSelectionEnd = FindWordEnd(mState.mSelectionEnd);
        break;
    }
    case TextEditor::SelectionMode::Line:
    {
        const auto lineNo = mState.mSelectionEnd.mLine;
        const auto lineSize = (size_t)lineNo < mLines.size() ? mLines[lineNo].size() : 0;
        mState.mSelectionStart = Coordinates(mState.mSelectionStart.mLine, 0);
        mState.mSelectionEnd = Coordinates(lineNo, (int)lineSize);
        break;
    }
    default:
        break;
    }

    if (mState.mSelectionStart != oldSelStart ||
        mState.mSelectionEnd != oldSelEnd)
    {
        mCursorPositionChanged = true;
    }
}
// ----------------------------------------------
//
// ----------------------------------------------
void TextEditor::InsertText(const CText& aValue)
{
    InsertText(aValue.str());
}
// ----------------------------------------------
//
// ----------------------------------------------
void TextEditor::InsertText(const CText::Char* aValue)
{
    if (aValue == nullptr)
    {
        return;
    }
    auto pos = GetActualCursorCoordinates();
    auto start = std::min(pos, mState.mSelectionStart);
    int totalLines = pos.mLine - start.mLine;

    totalLines += InsertTextAt(pos, aValue);

    SetSelection(pos, pos);
    SetCursorPosition(pos);
    Colorize(this, mLines);
}
// ----------------------------------------------
//
// ----------------------------------------------
void TextEditor::DeleteSelection()
{
    assert(mState.mSelectionEnd >= mState.mSelectionStart);

    if (mState.mSelectionEnd == mState.mSelectionStart)
        return;

    DeleteRange(mState.mSelectionStart, mState.mSelectionEnd);

    SetSelection(mState.mSelectionStart, mState.mSelectionStart);
    SetCursorPosition(mState.mSelectionStart);
    Colorize(this, mLines);
}
// ----------------------------------------------
//
// ----------------------------------------------
void TextEditor::MoveUp(int aAmount, bool aSelect)
{
    auto oldPos = mState.mCursorPosition;
    mState.mCursorPosition.mLine = std::max(0, mState.mCursorPosition.mLine - aAmount);
    if (oldPos != mState.mCursorPosition)
    {
        if (aSelect)
        {
            if (oldPos == mInteractiveStart)
                mInteractiveStart = mState.mCursorPosition;
            else if (oldPos == mInteractiveEnd)
                mInteractiveEnd = mState.mCursorPosition;
            else
            {
                mInteractiveStart = mState.mCursorPosition;
                mInteractiveEnd = oldPos;
            }
        }
        else
            mInteractiveStart = mInteractiveEnd = mState.mCursorPosition;
        SetSelection(mInteractiveStart, mInteractiveEnd);

        EnsureCursorVisible();
    }
}
// ----------------------------------------------
//
// ----------------------------------------------
void TextEditor::MoveDown(int aAmount, bool aSelect)
{
    assert(mState.mCursorPosition.mColumn >= 0);
    auto oldPos = mState.mCursorPosition;
    mState.mCursorPosition.mLine = std::max(0, std::min((int)mLines.size() - 1, mState.mCursorPosition.mLine + aAmount));

    if (mState.mCursorPosition != oldPos)
    {
        if (aSelect)
        {
            if (oldPos == mInteractiveEnd)
                mInteractiveEnd = mState.mCursorPosition;
            else if (oldPos == mInteractiveStart)
                mInteractiveStart = mState.mCursorPosition;
            else
            {
                mInteractiveStart = oldPos;
                mInteractiveEnd = mState.mCursorPosition;
            }
        }
        else
            mInteractiveStart = mInteractiveEnd = mState.mCursorPosition;
        SetSelection(mInteractiveStart, mInteractiveEnd);

        EnsureCursorVisible();
    }
}
// ----------------------------------------------
//
// ----------------------------------------------
void TextEditor::MoveLeft(int aAmount, bool aSelect, bool aWordMode)
{
    if (mLines.empty())
        return;

    auto oldPos = mState.mCursorPosition;
    mState.mCursorPosition = GetActualCursorCoordinates();

    while (aAmount-- > 0)
    {
        if (mState.mCursorPosition.mColumn == 0)
        {
            if (mState.mCursorPosition.mLine > 0)
            {
                --mState.mCursorPosition.mLine;
                mState.mCursorPosition.mColumn = (int)mLines[mState.mCursorPosition.mLine].size();
            }
        }
        else
        {
            mState.mCursorPosition.mColumn = std::max(0, mState.mCursorPosition.mColumn - 1);
            if (aWordMode)
                mState.mCursorPosition = FindWordStart(mState.mCursorPosition);
        }
    }

    assert(mState.mCursorPosition.mColumn >= 0);
    if (aSelect)
    {
        if (oldPos == mInteractiveStart)
            mInteractiveStart = mState.mCursorPosition;
        else if (oldPos == mInteractiveEnd)
            mInteractiveEnd = mState.mCursorPosition;
        else
        {
            mInteractiveStart = mState.mCursorPosition;
            mInteractiveEnd = oldPos;
        }
    }
    else
        mInteractiveStart = mInteractiveEnd = mState.mCursorPosition;
    SetSelection(mInteractiveStart, mInteractiveEnd, aSelect && aWordMode ? SelectionMode::Word : SelectionMode::Normal);

    EnsureCursorVisible();
}
// ----------------------------------------------
//
// ----------------------------------------------
void TextEditor::MoveRight(int aAmount, bool aSelect, bool aWordMode)
{
    auto oldPos = mState.mCursorPosition;

    if (mLines.empty())
        return;

    while (aAmount-- > 0)
    {
        auto& line = mLines[mState.mCursorPosition.mLine];
        if (mState.mCursorPosition.mColumn >= (int)line.size())
        {
            if (mState.mCursorPosition.mLine < (int)mLines.size() - 1)
            {
                mState.mCursorPosition.mLine = std::max(0, std::min((int)mLines.size() - 1, mState.mCursorPosition.mLine + 1));
                mState.mCursorPosition.mColumn = 0;
            }
        }
        else
        {
            mState.mCursorPosition.mColumn = std::max(0, std::min((int)line.size(), mState.mCursorPosition.mColumn + 1));
            if (aWordMode)
                mState.mCursorPosition = FindWordEnd(mState.mCursorPosition);
        }
    }

    if (aSelect)
    {
        if (oldPos == mInteractiveEnd)
            mInteractiveEnd = SanitizeCoordinates(mState.mCursorPosition);
        else if (oldPos == mInteractiveStart)
            mInteractiveStart = mState.mCursorPosition;
        else
        {
            mInteractiveStart = oldPos;
            mInteractiveEnd = mState.mCursorPosition;
        }
    }
    else
        mInteractiveStart = mInteractiveEnd = mState.mCursorPosition;
    SetSelection(mInteractiveStart, mInteractiveEnd, aSelect && aWordMode ? SelectionMode::Word : SelectionMode::Normal);

    EnsureCursorVisible();
}
// ----------------------------------------------
//
// ----------------------------------------------
void TextEditor::MoveTop(bool aSelect)
{
    auto oldPos = mState.mCursorPosition;
    SetCursorPosition(Coordinates(0, 0));

    if (mState.mCursorPosition != oldPos)
    {
        if (aSelect)
        {
            mInteractiveEnd = oldPos;
            mInteractiveStart = mState.mCursorPosition;
        }
        else
            mInteractiveStart = mInteractiveEnd = mState.mCursorPosition;
        SetSelection(mInteractiveStart, mInteractiveEnd);
    }
}
// ----------------------------------------------
//
// ----------------------------------------------
void TextEditor::TextEditor::MoveBottom(bool aSelect)
{
    auto oldPos = GetCursorPosition();
    auto newPos = Coordinates((int)mLines.size() - 1, 0);
    SetCursorPosition(newPos);
    if (aSelect)
    {
        mInteractiveStart = oldPos;
        mInteractiveEnd = newPos;
    }
    else
        mInteractiveStart = mInteractiveEnd = newPos;
    SetSelection(mInteractiveStart, mInteractiveEnd);
}
// ----------------------------------------------
//
// ----------------------------------------------
void TextEditor::MoveHome(bool aSelect)
{
    auto oldPos = mState.mCursorPosition;
    SetCursorPosition(Coordinates(mState.mCursorPosition.mLine, 0));

    if (mState.mCursorPosition != oldPos)
    {
        if (aSelect)
        {
            if (oldPos == mInteractiveStart)
                mInteractiveStart = mState.mCursorPosition;
            else if (oldPos == mInteractiveEnd)
                mInteractiveEnd = mState.mCursorPosition;
            else
            {
                mInteractiveStart = mState.mCursorPosition;
                mInteractiveEnd = oldPos;
            }
        }
        else
            mInteractiveStart = mInteractiveEnd = mState.mCursorPosition;
        SetSelection(mInteractiveStart, mInteractiveEnd);
    }
}
// ----------------------------------------------
//
// ----------------------------------------------
void TextEditor::MoveEnd(bool aSelect)
{
    auto oldPos = mState.mCursorPosition;
    SetCursorPosition(Coordinates(mState.mCursorPosition.mLine, (int)mLines[oldPos.mLine].size()));

    if (mState.mCursorPosition != oldPos)
    {
        if (aSelect)
        {
            if (oldPos == mInteractiveEnd)
                mInteractiveEnd = mState.mCursorPosition;
            else if (oldPos == mInteractiveStart)
                mInteractiveStart = mState.mCursorPosition;
            else
            {
                mInteractiveStart = oldPos;
                mInteractiveEnd = mState.mCursorPosition;
            }
        }
        else
            mInteractiveStart = mInteractiveEnd = mState.mCursorPosition;
        SetSelection(mInteractiveStart, mInteractiveEnd);
    }
}
// ----------------------------------------------
//
// ----------------------------------------------
void TextEditor::Delete()
{
    assert(!mReadOnly);

    if (mLines.empty())
        return;

    UndoRecord u;
    u.mBefore = mState;

    if (HasSelection())
    {
        u.mRemoved = GetSelectedText();
        u.mRemovedStart = mState.mSelectionStart;
        u.mRemovedEnd = mState.mSelectionEnd;

        DeleteSelection();
    }
    else
    {
        auto pos = GetActualCursorCoordinates();
        SetCursorPosition(pos);
        auto& line = mLines[pos.mLine];

        if (pos.mColumn == (int)line.size())
        {
            if (pos.mLine == (int)mLines.size() - 1)
                return;

            u.mRemoved = _T('\n');
            u.mRemovedStart = u.mRemovedEnd = GetActualCursorCoordinates();
            Advance(u.mRemovedEnd);

            auto& nextLine = mLines[pos.mLine + 1];
            line.insert(line.end(), nextLine.begin(), nextLine.end());
            RemoveLine(pos.mLine + 1);
        }
        else
        {
            u.mRemoved = line[pos.mColumn].mChar;
            u.mRemovedStart = u.mRemovedEnd = GetActualCursorCoordinates();
            u.mRemovedEnd.mColumn++;

            line.erase(line.begin() + pos.mColumn);
        }

        mTextChanged = true;

        Colorize(this, mLines);
    }

    u.mAfter = mState;
    AddUndo(u);
}
// ----------------------------------------------
//
// ----------------------------------------------
void TextEditor::BackSpace()
{
    assert(!mReadOnly);

    if (mLines.empty())
        return;

    UndoRecord u;
    u.mBefore = mState;

    if (HasSelection())
    {
        u.mRemoved = GetSelectedText();
        u.mRemovedStart = mState.mSelectionStart;
        u.mRemovedEnd = mState.mSelectionEnd;

        DeleteSelection();
    }
    else
    {
        auto pos = GetActualCursorCoordinates();
        SetCursorPosition(pos);

        if (mState.mCursorPosition.mColumn == 0)
        {
            if (mState.mCursorPosition.mLine == 0)
                return;

            u.mRemoved = _T('\n');
            u.mRemovedStart = u.mRemovedEnd = Coordinates(pos.mLine - 1, (int)mLines[pos.mLine - 1].size());
            Advance(u.mRemovedEnd);

            auto& line = mLines[mState.mCursorPosition.mLine];
            auto& prevLine = mLines[mState.mCursorPosition.mLine - 1];
            auto prevSize = (int)prevLine.size();
            prevLine.insert(prevLine.end(), line.begin(), line.end());

            RemoveLine(mState.mCursorPosition.mLine);
            --mState.mCursorPosition.mLine;
            mState.mCursorPosition.mColumn = prevSize;
        }
        else
        {
            auto& line = mLines[mState.mCursorPosition.mLine];

            u.mRemoved = line[pos.mColumn - 1].mChar;
            u.mRemovedStart = u.mRemovedEnd = GetActualCursorCoordinates();
            --u.mRemovedStart.mColumn;

            --mState.mCursorPosition.mColumn;
            if (mState.mCursorPosition.mColumn < (int)line.size())
                line.erase(line.begin() + mState.mCursorPosition.mColumn);
        }

        mTextChanged = true;

        EnsureCursorVisible();
        Colorize(this, mLines);
    }

    u.mAfter = mState;
    AddUndo(u);
}
// ----------------------------------------------
//
// ----------------------------------------------
void TextEditor::SelectWordUnderCursor()
{
    auto c = GetCursorPosition();
    SetSelection(FindWordStart(c), FindWordEnd(c));
}
// ----------------------------------------------
//
// ----------------------------------------------
void TextEditor::SelectAll()
{
    SetSelection(Coordinates(0, 0), Coordinates((int)mLines.size(), 0));
}
// ----------------------------------------------
//
// ----------------------------------------------
bool TextEditor::HasSelection() const
{
    return mState.mSelectionEnd > mState.mSelectionStart;
}
// ----------------------------------------------
//
// ----------------------------------------------
void TextEditor::Copy()
{
    if (HasSelection())
    {
        ImGui::SetClipboardText((char*)GetSelectedText().toSingle().str());
    }
    else
    {
        if (!mLines.empty())
        {
            CText str;
            auto& line = mLines[GetActualCursorCoordinates().mLine];
            for (auto& g : line)
            {
                str.push_back(g.mChar);
            }
            ImGui::SetClipboardText((char*)str.str());
        }
    }
}
// ----------------------------------------------
//
// ----------------------------------------------
void TextEditor::Cut()
{
    if (IsReadOnly())
    {
        Copy();
    }
    else
    {
        if (HasSelection())
        {
            UndoRecord u;
            u.mBefore = mState;
            u.mRemoved = GetSelectedText();
            u.mRemovedStart = mState.mSelectionStart;
            u.mRemovedEnd = mState.mSelectionEnd;

            Copy();
            DeleteSelection();

            u.mAfter = mState;
            AddUndo(u);
        }
    }
}
// ----------------------------------------------
//
// ----------------------------------------------
void TextEditor::Paste()
{
    auto clipText = ImGui::GetClipboardText();
    if (clipText != nullptr && strlen(clipText) > 0)
    {
        UndoRecord u;
        u.mBefore = mState;

        if (HasSelection())
        {
            u.mRemoved = GetSelectedText();
            u.mRemovedStart = mState.mSelectionStart;
            u.mRemovedEnd = mState.mSelectionEnd;
            DeleteSelection();
        }

        u.mAdded.fromSingle(clipText);
        u.mAddedStart = GetActualCursorCoordinates();

        InsertText(u.mAdded);

        u.mAddedEnd = GetActualCursorCoordinates();
        u.mAfter = mState;
        AddUndo(u);
    }
}
// ----------------------------------------------
//
// ----------------------------------------------
bool TextEditor::CanUndo() const
{
    return mUndoIndex > 0;
}
// ----------------------------------------------
//
// ----------------------------------------------
bool TextEditor::CanRedo() const
{
    return mUndoIndex < (int)mUndoBuffer.size();
}
// ----------------------------------------------
//
// ----------------------------------------------
void TextEditor::Undo(int aSteps)
{
    while (CanUndo() && aSteps-- > 0)
        mUndoBuffer[--mUndoIndex].Undo(this);
}
// ----------------------------------------------
//
// ----------------------------------------------
void TextEditor::Redo(int aSteps)
{
    while (CanRedo() && aSteps-- > 0)
        mUndoBuffer[mUndoIndex++].Redo(this);
}
// ----------------------------------------------
//
// ----------------------------------------------
CText TextEditor::GetText() const
{
    return GetText(Coordinates(), Coordinates((int)mLines.size(), 0));
}
// ----------------------------------------------
//
// ----------------------------------------------
std::vector<CText> TextEditor::GetTextLines() const
{
    std::vector<CText> result;

    result.reserve(mLines.size());

    for (auto& line : mLines)
    {
        CText text;

        text.resize(line.size());

        for (size_t i = 0; i < line.size(); ++i)
            text[i] = line[i].mChar;

        result.emplace_back(std::move(text));
    }

    return result;
}
// ----------------------------------------------
//
// ----------------------------------------------
CText TextEditor::GetSelectedText() const
{
    return GetText(mState.mSelectionStart, mState.mSelectionEnd);
}
// ----------------------------------------------
//
// ----------------------------------------------
CText TextEditor::GetCurrentLineText()const
{
    auto lineLength = (int)mLines[mState.mCursorPosition.mLine].size();
    return GetText(Coordinates(mState.mCursorPosition.mLine, 0), Coordinates(mState.mCursorPosition.mLine, lineLength));
}
// ----------------------------------------------
//
// ----------------------------------------------
void TextEditor::ProcessInputs()
{
}
// ----------------------------------------------
//
// ----------------------------------------------
/*
void TextEditor::Colorize(int aFromLine, int aLines)
{
    int toLine = aLines == -1 ? (int)mLines.size() : std::min((int)mLines.size(), aFromLine + aLines);
    mColorRangeMin = std::min(mColorRangeMin, aFromLine);
    mColorRangeMax = std::max(mColorRangeMax, toLine);
    mColorRangeMin = std::max(0, mColorRangeMin);
    mColorRangeMax = std::max(mColorRangeMin, mColorRangeMax);
}
*/
/*
// ----------------------------------------------
//
// ----------------------------------------------
void TextEditor::ColorizeRange(int aFromLine, int aToLine)
{
    if (mLines.empty())
        return;
    for (auto line : mLines)
    {
        line[0].mColor = ImVec4(255, 255, 0, 255);
    }
}
// ----------------------------------------------
//
// ----------------------------------------------
void TextEditor::ColorizeInternal()
{

}
*/
// ----------------------------------------------
//
// ----------------------------------------------
float TextEditor::TextDistanceToLineStart(const Coordinates& aFrom) const
{
    auto& line = mLines[aFrom.mLine];
    float distance = 0.0f;
    auto fontScale = 1.0f;// ImGui::GetFontSize() / ImGui::GetFont()->FontSize;
    float spaceSize = font->CalcTextSizeA(font->FontSize, FLT_MAX, -1.0f, (char*)" ", nullptr, nullptr).x;
    for (size_t it = 0u; it < line.size() && it < (unsigned)aFrom.mColumn; ++it)
    {
        if (line[it].mChar == _T('\t'))
        {
            distance = (1.0f * fontScale + std::floor((1.0f + distance)) / (float(mTabSize) * spaceSize)) * (float(mTabSize) * spaceSize);
        }
        else
        {
            CText tempCString;
            tempCString.append(line[it].mChar);
            tempCString.append(_T('\0'));
            //tempCString[0] = line[it].mChar;
            //tempCString[1] = _T(\0';
            std::string bstr;
            bstr = tempCString.toSingle().str();
            //wstrtostr(tempCString);
            distance += font->CalcTextSizeA(font->FontSize, FLT_MAX, -1.0f, (char*)bstr.c_str(), nullptr, nullptr).x;
        }
    }

    return distance;
}
// ----------------------------------------------
//
// ----------------------------------------------
void TextEditor::EnsureCursorVisible()
{
    if (!mWithinRender)
    {
        mScrollToCursor = true;
        return;
    }
    float scrollX = ImGui::GetScrollX();
    float scrollY = ImGui::GetScrollY();
    auto height = ImGui::GetWindowHeight();
    auto width = ImGui::GetWindowWidth();
    auto top = 1 + (int)ceil(scrollY / mCharAdvance.y);
    auto bottom = (int)ceil((scrollY + height) / mCharAdvance.y);
    auto left = (int)ceil(scrollX / mCharAdvance.x);
    auto right = (int)ceil((scrollX + width) / mCharAdvance.x);
    auto pos = GetActualCursorCoordinates();
    auto len = TextDistanceToLineStart(pos);
    if (pos.mLine < top)
    {
        ImGui::SetScrollY(std::max(0.0f, (pos.mLine - 1) * mCharAdvance.y));
    }
    if (pos.mLine > bottom - 4)
    {
        ImGui::SetScrollY(std::max(0.0f, (pos.mLine + 4) * mCharAdvance.y - height));
    }
    if (len + mTextStart < left + 4)
    {
        ImGui::SetScrollX(std::max(0.0f, len + mTextStart - 4));
    }
    if (len + mTextStart > right - 4)
    {
        ImGui::SetScrollX(std::max(0.0f, len + mTextStart + 4 - width));
    }
}
// ----------------------------------------------
//
// ----------------------------------------------
int TextEditor::GetPageSize() const
{
    auto height = ImGui::GetWindowHeight() - 20.0f;
    return (int)floor(height / mCharAdvance.y);
}
// ----------------------------------------------
//
// ----------------------------------------------
TextEditor::UndoRecord::UndoRecord(
    const CText& aAdded,
    const TextEditor::Coordinates aAddedStart,
    const TextEditor::Coordinates aAddedEnd,
    const CText& aRemoved,
    const TextEditor::Coordinates aRemovedStart,
    const TextEditor::Coordinates aRemovedEnd,
    TextEditor::EditorState& aBefore,
    TextEditor::EditorState& aAfter)
    : mAdded(aAdded)
    , mAddedStart(aAddedStart)
    , mAddedEnd(aAddedEnd)
    , mRemoved(aRemoved)
    , mRemovedStart(aRemovedStart)
    , mRemovedEnd(aRemovedEnd)
    , mBefore(aBefore)
    , mAfter(aAfter)
{
    assert(mAddedStart <= mAddedEnd);
    assert(mRemovedStart <= mRemovedEnd);
}
// ----------------------------------------------
//
// ----------------------------------------------
void TextEditor::UndoRecord::Undo(TextEditor* aEditor)
{
    if (!mAdded.isEmpty())
    {
        aEditor->DeleteRange(mAddedStart, mAddedEnd);
        aEditor->Colorize(aEditor,aEditor->mLines);
    }
    if (!mRemoved.isEmpty())
    {
        auto start = mRemovedStart;
        aEditor->InsertTextAt(start, mRemoved.str());
        aEditor->Colorize(aEditor, aEditor->mLines);
    }
    aEditor->mState = mBefore;
    aEditor->EnsureCursorVisible();
}
// ----------------------------------------------
//
// ----------------------------------------------
void TextEditor::UndoRecord::Redo(TextEditor* aEditor)
{
    if (!mRemoved.isEmpty())
    {
        aEditor->DeleteRange(mRemovedStart, mRemovedEnd);
        aEditor->Colorize(aEditor, aEditor->mLines);
    }
    if (!mAdded.isEmpty())
    {
        auto start = mAddedStart;
        aEditor->InsertTextAt(start, mAdded.str());
        aEditor->Colorize(aEditor, aEditor->mLines);
    }
    aEditor->mState = mAfter;
    aEditor->EnsureCursorVisible();
}
