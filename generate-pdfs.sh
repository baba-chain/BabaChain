#!/bin/bash
# generate-pdfs.sh

echo "Generating BabaChain Whitepapers..."

# English PDF
pandoc whitepaper.md -o "BabaChain_Whitepaper.pdf" \
  --pdf-engine=xelatex \
  --variable mainfont="Times New Roman" \
  --variable fontsize=11pt \
  --variable geometry:margin=1in \
  --toc \
  --number-sections \
  --highlight-style=tango

# Turkish PDF  
pandoc whitepaper-tr.md -o "BabaChain_Teknik_Raporu.pdf" \
  --pdf-engine=xelatex \
  --variable mainfont="Times New Roman" \
  --variable fontsize=11pt \
  --variable geometry:margin=1in \
  --toc \
  --number-sections \
  --highlight-style=tango

echo "PDFs generated successfully!"
