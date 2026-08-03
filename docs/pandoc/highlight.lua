-- Evidenzia gli span con classe .highlight in LaTeX/PDF
function Span(el)
  for _, c in ipairs(el.classes) do
    if c == "highlight" then
      local txt = pandoc.utils.stringify(el.content)
      return pandoc.RawInline("latex", "\\highlight{" .. txt .. "}")
    end
  end
  return el
end
