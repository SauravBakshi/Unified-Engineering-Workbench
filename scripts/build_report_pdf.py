#!/usr/bin/env python3
"""Render docs/report.md as a compact PDF using ReportLab (no Pandoc/LaTeX)."""
from pathlib import Path
import re
from xml.sax.saxutils import escape
from reportlab.lib import colors
from reportlab.lib.enums import TA_CENTER
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.lib.units import mm
from reportlab.platypus import SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle, ListFlowable, ListItem, KeepTogether

ROOT = Path(__file__).resolve().parents[1]
source = (ROOT / 'docs' / 'report.md').read_text(encoding='utf-8').splitlines()
out = ROOT / 'docs' / 'report.pdf'
styles = getSampleStyleSheet()
styles.add(ParagraphStyle(name='ReportTitle', parent=styles['Title'], fontName='Helvetica-Bold', fontSize=17, leading=21, textColor=colors.HexColor('#173c64'), alignment=TA_CENTER, spaceAfter=7))
styles.add(ParagraphStyle(name='H2x', parent=styles['Heading2'], fontSize=11.5, leading=14, textColor=colors.HexColor('#173c64'), spaceBefore=8, spaceAfter=4))
styles.add(ParagraphStyle(name='Bodyx', parent=styles['BodyText'], fontSize=8.5, leading=11, spaceAfter=4))
styles.add(ParagraphStyle(name='Smallx', parent=styles['BodyText'], fontSize=7.4, leading=9))

def inline(text):
    text = escape(text)
    text = re.sub(r'\*\*(.+?)\*\*', r'<b>\1</b>', text)
    text = re.sub(r'`(.+?)`', r'<font name="Courier">\1</font>', text)
    return text

story=[]
i=0
while i < len(source):
    line=source[i].strip()
    if not line:
        i+=1; continue
    if line.startswith('# '):
        story.append(Paragraph(inline(line[2:]), styles['ReportTitle'])); i+=1; continue
    if line.startswith('## '):
        story.append(Paragraph(inline(line[3:]), styles['H2x'])); i+=1; continue
    if line.startswith('|'):
        block=[]
        while i<len(source) and source[i].strip().startswith('|'):
            cells=[c.strip() for c in source[i].strip().strip('|').split('|')]
            if not all(re.fullmatch(r':?-{2,}:?',c) for c in cells): block.append(cells)
            i+=1
        if block:
            data=[[Paragraph(inline(c), styles['Smallx']) for c in row] for row in block]
            widths=[(A4[0]-34*mm)/len(block[0])]*len(block[0])
            table=Table(data,colWidths=widths,repeatRows=1,hAlign='LEFT')
            table.setStyle(TableStyle([
                ('BACKGROUND',(0,0),(-1,0),colors.HexColor('#e8f0f8')),
                ('TEXTCOLOR',(0,0),(-1,0),colors.HexColor('#173c64')),
                ('GRID',(0,0),(-1,-1),.35,colors.HexColor('#ccd7e2')),
                ('VALIGN',(0,0),(-1,-1),'TOP'),
                ('LEFTPADDING',(0,0),(-1,-1),4),('RIGHTPADDING',(0,0),(-1,-1),4),
                ('TOPPADDING',(0,0),(-1,-1),4),('BOTTOMPADDING',(0,0),(-1,-1),4),
            ]))
            story.extend([Spacer(1,3),table,Spacer(1,4)])
        continue
    if line.startswith('- '):
        items=[]
        while i<len(source) and source[i].strip().startswith('- '):
            items.append(ListItem(Paragraph(inline(source[i].strip()[2:]),styles['Bodyx']),leftIndent=8))
            i+=1
        story.append(ListFlowable(items,bulletType='bullet',start='circle',leftIndent=13,bulletFontSize=5)); continue
    # Join wrapped Markdown lines into one paragraph.
    para=[line]; i+=1
    while i<len(source) and source[i].strip() and not source[i].strip().startswith(('#','|','- ')):
        para.append(source[i].strip()); i+=1
    story.append(Paragraph(inline(' '.join(para)),styles['Bodyx']))

def footer(canvas, doc):
    canvas.saveState(); canvas.setFont('Helvetica',7); canvas.setFillColor(colors.HexColor('#64788d'))
    canvas.drawString(17*mm,10*mm,'Unified Engineering Workbench · Technical Report · Linux audit; Windows gate pending')
    canvas.drawRightString(A4[0]-17*mm,10*mm,f'Page {doc.page}')
    canvas.restoreState()

doc=SimpleDocTemplate(str(out),pagesize=A4,rightMargin=17*mm,leftMargin=17*mm,topMargin=15*mm,bottomMargin=17*mm,title='Unified Engineering Workbench Technical Report',author='Candidate submission draft')
doc.build(story,onFirstPage=footer,onLaterPages=footer)
print(out)
