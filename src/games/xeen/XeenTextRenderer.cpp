#include "games/xeen/XeenTextRenderer.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <array>
#include <string_view>
#include "XeenDialogDrawing.inc"

namespace mmodern {
namespace {

struct GlyphAtom {
	std::uint8_t character = 0;
	XeenFontSize size = XeenFontSize::Normal;
	std::uint8_t colorIndex = 0;
	std::size_t sourceOffset = 0;
};

struct TextLine {
	std::vector<GlyphAtom> glyphs;
	int width = 0;
	int height = 0;
	XeenTextAlignment alignment = XeenTextAlignment::Left;
};

std::uint8_t paletteColor(std::uint8_t index) {
	static constexpr std::uint8_t colors[40] = {
		0x19, 0x08, 0x0f, 0x15, 0x01, 0x21, 0x26, 0x2b, 0x31, 0x36,
		0x3d, 0x41, 0x46, 0x4c, 0x50, 0x55, 0x5d, 0x60, 0x65, 0x6c,
		0x70, 0x75, 0x7b, 0x80, 0x85, 0x8d, 0x90, 0x97, 0x9d, 0xa4,
		0xab, 0xb0, 0xb6, 0xbd, 0xc0, 0xc6, 0xcd, 0xd0, 0xd6, 0xdb
	};
	return colors[index < 40 ? index : 0];
}

bool parseDigits(const std::string &text, std::size_t &offset, int count, int &value) {
	if (offset + static_cast<std::size_t>(count) > text.size())
		return false;
	value = 0;
	for (int i = 0; i < count; ++i) {
		const unsigned char c = static_cast<unsigned char>(text[offset++]);
		if (c < '0' || c > '9')
			return false;
		value = value * 10 + c - '0';
	}
	return true;
}

std::string controlDiagnostic(unsigned value) {
	std::ostringstream output;
	output << "Unsupported Xeen text control: 0x" << std::hex
		<< std::setw(2) << std::setfill('0') << value;
	return output.str();
}

void setPixel(IndexedFrame &frame, int x, int y, std::uint8_t color,
		const XeenTextRect &clip) {
	if (x < clip.left || x >= clip.right || y < clip.top || y >= clip.bottom ||
			x < 0 || x >= frame.width || y < 0 || y >= frame.height)
		return;
	frame.pixels[static_cast<std::size_t>(y) * frame.width + x] = color;
}

void fillRect(IndexedFrame &frame, const XeenTextRect &rect, std::uint8_t color) {
	const int left = std::max(0, rect.left);
	const int top = std::max(0, rect.top);
	const int right = std::min(frame.width, rect.right);
	const int bottom = std::min(frame.height, rect.bottom);
	for (int y = top; y < bottom; ++y) {
		std::fill(frame.pixels.begin() + static_cast<std::size_t>(y) * frame.width + left,
			frame.pixels.begin() + static_cast<std::size_t>(y) * frame.width + right, color);
	}
}

void drawWindow(IndexedFrame &frame, const XeenTextRect &bounds) {
	fillRect(frame, bounds, 0x99);
	for (int i = 0; i < 3; ++i) {
		for (int x = bounds.left + i; x < bounds.right - i; ++x) {
			setPixel(frame, x, bounds.top + i, 0xa4, bounds);
			setPixel(frame, x, bounds.bottom - 1 - i, 0x97, bounds);
		}
		for (int y = bounds.top + i; y < bounds.bottom - i; ++y) {
			setPixel(frame, bounds.left + i, y, 0xa4, bounds);
			setPixel(frame, bounds.right - 1 - i, y, 0x97, bounds);
		}
	}
}

// ScummVM developers, GPL-3.0-or-later, pinned window.cpp/font.cpp.
void originalWindow(IndexedFrame &frame, const XeenTextRect &b) {
	const auto symbol = [&](unsigned id, int x, int y) {
		for (int gy=0; gy<8; ++gy) for (int gx=0; gx<8; ++gx)
			if(const auto color=generated_dialog_drawing::kWindowSymbols[id][gy*8+gx]) setPixel(frame,x+gx,y+gy,color,b);
	};
	symbol(0,b.left,b.top); symbol(5,b.right-8,b.top);
	symbol(14,b.left,b.bottom-8); symbol(19,b.right-8,b.bottom-8);
	for (int i=0; i<(b.right-b.left-9)/8; ++i) {
		symbol(1+i%4,b.left+8+i*8,b.top); symbol(15+i%4,b.left+8+i*8,b.bottom-8);
	}
	for (int i=0; i<(b.bottom-b.top-9)/8; ++i) {
		symbol(6+i%4,b.left,b.top+8+i*8); symbol(10+i%4,b.right-8,b.top+8+i*8);
	}
	fillRect(frame,{b.left+8,b.top+8,b.right-8,b.bottom-8},0x99);
}

XeenTextRenderResult renderOriginal(const IndexedFrame &base, const std::string &text,
        const XeenTextRenderOptions &o, const XeenFontFormat &font) {
    XeenTextRenderResult result;
    IndexedFrame frame=base;
    if(o.drawWindow) originalWindow(frame,o.windowBounds);
    int x=o.x,y=o.y; unsigned color=o.colorIndex,bg=0x99;
    auto size=o.size; auto alignment=o.alignment;
    std::size_t i=0;
    const auto number=[&](int digits) {
        int value=0;
        // FontSurface::fontAtoi consumes spaces as zero and stops immediately
        // after the first non-digit (including the default-color 'd').
        for(int n=0;n<digits;++n) {
            if(i==text.size()) return -1;
            const unsigned char c=text[i++]&0x7f;
            const int digit=c==' '?0:int(c)-'0';
            if(digit<0 || digit>9) return -1;
            value=value*10+digit;
        }
        return value;
    };
    const auto glyph=[&](unsigned char c,bool outline=false) {
        const auto pixels=font.glyph(c,size);
        const int dy=(c=='g'||c=='p'||c=='q'||c=='y')?1:0;
        for(int gy=0;gy<8;++gy) for(int gx=0;gx<8;++gx) {
            const auto shade=pixels.pixels[gy*8+gx];
            if(shade) setPixel(frame,x+gx,y+gy+dy,outline?bg:
                (o.startupColors?generated_dialog_drawing::kStartupTextColors:generated_dialog_drawing::kTextColors).at(color)[shade],o.bounds);
        }
        if(y+8>o.bounds.bottom) result.diagnostics.push_back("Original dialog text clipped vertically");
        x+=font.advance(c,size);
    };
    const auto newline=[&] {
        while(i<text.size() && (text[i]&0x7f)==' ') ++i;
        x=o.bounds.left;y+=size==XeenFontSize::Reduced?9:10;
    };
    // FontSurface::writeString scans a run through palette changes, but stops
    // at each other command. Commands alter the next run's cursor/font state.
    while(i<text.size()) {
        const auto start=i;std::size_t end=i;
        int xp=alignment==XeenTextAlignment::Center?o.bounds.left:x;
        bool wraps=false;
        for(;end<text.size();) {
            const auto c=static_cast<unsigned char>(text[end])&0x7f;
            if(c>=32) {xp+=c==' '?4:font.advance(c,size);++end;}
            else if(c==12) {end+=end+1<text.size() && text[end+1]=='d'?2:3;continue;}
            else if(c==8 && end+1<text.size() && text[end+1]==' ') {xp-=2;end+=2;continue;}
            else break;
            if(xp>=o.bounds.right) {--end;wraps=true;break;}
        }
        if(wraps && alignment!=XeenTextAlignment::Right) {
            auto space=end;while(space>start && (text[space]&0x7f)!=' ') --space;
            if(space>start) end=space;
            else if(alignment==XeenTextAlignment::Left && x!=o.bounds.left) {newline();continue;}
            else if(end>start) --end;
        }
        if(alignment!=XeenTextAlignment::Left) {
            int width=0;
            for(auto scan=start;scan<text.size() && scan<=end;) {
                const auto c=static_cast<unsigned char>(text[scan++])&0x7f;
                if(c>=32) width+=c==' '?4:font.advance(c,size);
                else if(c==12) {scan+=scan<text.size() && text[scan]=='d'?1:2;}
                else if(c==8 && scan<text.size() && text[scan]==' ') {--width;++scan;}
                else break;
            }
            if(end<text.size() && text[end]==' ') width-=size==XeenFontSize::Reduced?4:5;
            if(alignment==XeenTextAlignment::Right) x=(x==o.bounds.left?o.bounds.right:x)-width-1;
            else x=x==o.bounds.left?(o.bounds.left+o.bounds.right+1-width)/2:(x*2-width)/2;
        }
        while(i<text.size() && i<=end) {
            const unsigned char c=text[i++]&0x7f;
            if(c>=32) {if(c==' ') x+=font.advance(c,size);else glyph(c);continue;}
            switch(c) {
            case 1:size=XeenFontSize::Normal;break;
            case 2:size=XeenFontSize::Reduced;break;
            case 3:
                if(i==text.size()) throw std::invalid_argument("Truncated original alignment");
                alignment=text[i]=='c'?XeenTextAlignment::Center:text[i]=='r'?XeenTextAlignment::Right:XeenTextAlignment::Left;++i;break;
            case 4:{const int width=number(3);const int left=x-(alignment==XeenTextAlignment::Right?width:0);fillRect(frame,{left,y,left+width,y+(size==XeenFontSize::Reduced?9:10)},bg);break;}
            case 5:break;
            case 6:glyph(' ');break;
            case 7:bg=number(3);if(bg>255)bg=0x99;break;
            case 8:{
                if(i==text.size()) throw std::invalid_argument("Truncated original outline");
                unsigned char ch=text[i++]&0x7f;
                if(ch==' ') x=std::max(o.bounds.left,x-3);
                else {if(ch==6)ch=' ';x=std::max(o.bounds.left,x-font.advance(ch,size));const int old=x;glyph(ch,true);x=old;}
                break;
            }
            case 9:x=std::min(o.bounds.left+number(3),o.bounds.right);break;
            case 10:newline();break;
            case 11:y=std::min(o.bounds.top+number(3),o.bounds.bottom);break;
            case 12:
                {const int index=number(2);color=index<0?0:unsigned(index);}
                if(color>=40)throw std::invalid_argument("Invalid original text palette");break;
            case 13:fillRect(frame,o.bounds,bg);x=o.bounds.left;y=o.bounds.top;break;
            default:throw std::invalid_argument("Unsupported original dialog control");
            }
        }
        if(wraps && alignment!=XeenTextAlignment::Right) newline();
    }
    result.pages.push_back(std::move(frame));result.pageSourceEnds.push_back(text.size());return result;
}

std::vector<GlyphAtom> parseText(const std::string &text,
		const XeenTextRenderOptions &options, std::vector<std::string> &diagnostics,
		std::vector<std::size_t> &breaks, std::vector<XeenTextAlignment> &alignments) {
	std::vector<GlyphAtom> glyphs;
	XeenFontSize size = options.size;
	std::uint8_t color = options.colorIndex;
	XeenTextAlignment alignment = options.alignment;
	alignments.push_back(alignment);
	for (std::size_t i = 0; i < text.size();) {
		const unsigned char raw = static_cast<unsigned char>(text[i++]);
		const unsigned char c = raw & 0x7f;
		if (c >= ' ') {
			glyphs.push_back({c, size, color, i - 1});
			continue;
		}
		switch (c) {
		case 1: size = XeenFontSize::Normal; break;
		case 2: size = XeenFontSize::Reduced; break;
		case 3:
			if (i >= text.size()) {
				diagnostics.push_back("Truncated Xeen alignment control");
				break;
			}
			switch (text[i++] & 0x7f) {
			case 'c': alignment = XeenTextAlignment::Center; break;
			case 'r': alignment = XeenTextAlignment::Right; break;
			default: alignment = XeenTextAlignment::Left; break;
			}
			alignments.back() = alignment;
			break;
		case 5: break;
		case 6: glyphs.push_back({' ', size, color, i - 1}); break;
		case 7: {
			int ignored = 0;
			if (!parseDigits(text, i, 3, ignored))
				diagnostics.push_back("Invalid Xeen background control");
			break;
		}
		case 9: {
			int ignored = 0;
			if (!parseDigits(text, i, 3, ignored))
				diagnostics.push_back("Invalid Xeen X-position control");
			break;
		}
		case 10:
			breaks.push_back(glyphs.size());
			alignments.push_back(alignment);
			break;
		case 11: {
			int ignored = 0;
			if (!parseDigits(text, i, 3, ignored))
				diagnostics.push_back("Invalid Xeen Y-position control");
			break;
		}
		case 12:
			if (i < text.size() && (text[i] & 0x7f) == 'd') {
				++i;
				color = 0;
			} else {
				int value = 0;
				if (parseDigits(text, i, 2, value) && value < 40)
					color = static_cast<std::uint8_t>(value);
				else
					diagnostics.push_back("Invalid Xeen color control");
			}
			break;
		case 13:
			glyphs.clear();
			breaks.clear();
			alignments.assign(1, alignment);
			break;
		case 4:
			diagnostics.push_back(controlDiagnostic(c));
			if (i + 3 <= text.size()) i += 3; else i = text.size();
			break;
		case 8:
			diagnostics.push_back(controlDiagnostic(c));
			break;
		default:
			diagnostics.push_back(controlDiagnostic(c));
			return glyphs;
		}
	}
	return glyphs;
}

} // namespace

XeenTextRenderer::XeenTextRenderer(const XeenFontFormat &font) : _font(font) {
}

int XeenTextRenderer::textWidth(const std::string &text, XeenFontSize size,
		std::vector<std::string> *diagnostics) const {
	XeenTextRenderOptions options;
	options.size = size;
	std::vector<std::string> localDiagnostics;
	std::vector<std::size_t> breaks;
	std::vector<XeenTextAlignment> alignments;
	const auto glyphs = parseText(text, options, localDiagnostics, breaks, alignments);
	int width = 0;
	int maximum = 0;
	std::size_t breakIndex = 0;
	for (std::size_t i = 0; i <= glyphs.size(); ++i) {
		if ((breakIndex < breaks.size() && i == breaks[breakIndex]) || i == glyphs.size()) {
			maximum = std::max(maximum, width);
			width = 0;
			++breakIndex;
		}
		if (i < glyphs.size())
			width += _font.advance(glyphs[i].character, glyphs[i].size);
	}
	if (diagnostics)
		diagnostics->insert(diagnostics->end(), localDiagnostics.begin(), localDiagnostics.end());
	return maximum;
}

XeenTextRenderResult XeenTextRenderer::render(const IndexedFrame &base,
		const std::string &text, const XeenTextRenderOptions &options) const {
	if (!base.isValid())
		throw std::invalid_argument("Invalid framebuffer for Xeen text");
	if (options.bounds.left < 0 || options.bounds.top < 0 ||
			options.bounds.right > base.width || options.bounds.bottom > base.height ||
			options.bounds.left >= options.bounds.right ||
			options.bounds.top >= options.bounds.bottom)
		throw std::invalid_argument("Invalid Xeen text bounds");

	if (options.originalControls) return renderOriginal(base,text,options,_font);
	XeenTextRenderResult result;
	std::vector<std::size_t> explicitBreaks;
	std::vector<XeenTextAlignment> alignments;
	const auto glyphs = parseText(text, options, result.diagnostics,
		explicitBreaks, alignments);
	const int availableWidth = options.bounds.right - options.bounds.left;
	std::vector<TextLine> lines(1);
	lines.back().alignment = alignments.empty() ? options.alignment : alignments[0];
	std::size_t nextBreak = 0;
	std::size_t alignmentIndex = 1;
	for (std::size_t i = 0; i < glyphs.size(); ++i) {
		while (nextBreak < explicitBreaks.size() && i == explicitBreaks[nextBreak]) {
			lines.emplace_back();
			lines.back().alignment = alignmentIndex < alignments.size() ?
				alignments[alignmentIndex++] : options.alignment;
			++nextBreak;
		}
		const GlyphAtom atom = glyphs[i];
		const int advance = _font.advance(atom.character, atom.size);
		const bool wordStart = atom.character != ' ' &&
			(i == 0 || glyphs[i - 1].character == ' ' ||
				std::find(explicitBreaks.begin(), explicitBreaks.end(), i) != explicitBreaks.end());
		if (wordStart && !lines.back().glyphs.empty()) {
			int wordWidth = 0;
			for (std::size_t word = i; word < glyphs.size() &&
					glyphs[word].character != ' ' &&
					std::find(explicitBreaks.begin(), explicitBreaks.end(), word) ==
						explicitBreaks.end(); ++word) {
				wordWidth += _font.advance(glyphs[word].character, glyphs[word].size);
			}
			if (lines.back().width + wordWidth > availableWidth) {
				while (!lines.back().glyphs.empty() &&
						lines.back().glyphs.back().character == ' ') {
					lines.back().width -= _font.advance(' ', lines.back().glyphs.back().size);
					lines.back().glyphs.pop_back();
				}
				lines.emplace_back();
				lines.back().alignment = lines[lines.size() - 2].alignment;
			}
		}
		if (!lines.back().glyphs.empty() && lines.back().width + advance > availableWidth) {
			while (!lines.back().glyphs.empty() &&
					lines.back().glyphs.back().character == ' ') {
				lines.back().width -= _font.advance(' ', lines.back().glyphs.back().size);
				lines.back().glyphs.pop_back();
			}
			lines.emplace_back();
			lines.back().alignment = lines[lines.size() - 2].alignment;
			if (atom.character == ' ')
				continue;
		}
		lines.back().glyphs.push_back(atom);
		lines.back().width += advance;
		lines.back().height = std::max(lines.back().height,
			atom.size == XeenFontSize::Reduced ? 9 : 10);
	}
	while (nextBreak < explicitBreaks.size() && explicitBreaks[nextBreak] == glyphs.size()) {
		lines.emplace_back();
		lines.back().alignment = alignmentIndex < alignments.size() ?
			alignments[alignmentIndex++] : options.alignment;
		++nextBreak;
	}

	const int availableHeight = options.bounds.bottom - options.y;
	const int linesPerPage = options.paginate ? std::max(1, availableHeight / 10) :
		static_cast<int>(lines.size());
	for (std::size_t first = 0; first < lines.size(); first += linesPerPage) {
		IndexedFrame frame = base;
		if (options.drawWindow) {
			const XeenTextRect window = options.windowBounds.right > options.windowBounds.left ?
				options.windowBounds : options.bounds;
			drawWindow(frame, window);
		}
		int y = options.y;
		const std::size_t end = std::min(lines.size(), first + linesPerPage);
		for (std::size_t lineIndex = first; lineIndex < end; ++lineIndex) {
			const TextLine &line = lines[lineIndex];
			int x = options.x;
			if (line.alignment == XeenTextAlignment::Center)
				x = options.alignmentAnchor >= 0 ?
					(2 * options.alignmentAnchor - line.width) / 2 :
					(options.bounds.left + options.bounds.right - line.width) / 2;
			else if (line.alignment == XeenTextAlignment::Right)
				x = options.bounds.right - line.width;
			for (const GlyphAtom &atom : line.glyphs) {
				if (atom.character != ' ') {
					const XeenFontGlyph glyph = _font.glyph(atom.character, atom.size);
					const int glyphY = y + ((atom.character == 'g' || atom.character == 'p' ||
						atom.character == 'q' || atom.character == 'y') ? 1 : 0);
					for (int gy = 0; gy < XeenFontGlyph::kHeight; ++gy) {
						for (int gx = 0; gx < XeenFontGlyph::kWidth; ++gx) {
							if (glyph.pixels[static_cast<std::size_t>(gy) * 8 + gx])
								setPixel(frame, x + gx, glyphY + gy,
									paletteColor(atom.colorIndex), options.bounds);
						}
					}
				}
				x += _font.advance(atom.character, atom.size);
			}
			y += line.height ? line.height : 10;
		}
		result.pages.push_back(std::move(frame));
		std::size_t sourceEnd = text.size();
		for (std::size_t next = end; next < lines.size(); ++next) {
			if (!lines[next].glyphs.empty()) {
				sourceEnd = lines[next].glyphs.front().sourceOffset;
				break;
			}
		}
		result.pageSourceEnds.push_back(sourceEnd);
	}
	if (result.pages.empty()) {
		result.pages.push_back(base);
		result.pageSourceEnds.push_back(text.size());
		if (options.drawWindow) {
			const XeenTextRect window = options.windowBounds.right > options.windowBounds.left ?
				options.windowBounds : options.bounds;
			drawWindow(result.pages.back(), window);
		}
	}
	return result;
}

} // namespace mmodern
