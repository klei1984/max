source "https://rubygems.org"

# Mirrors the gem set GitHub Pages builds with. Check the live versions at
# https://pages.github.com/versions/ and bump this when GitHub moves.
# github-pages 232 => jekyll 3.10.0, and already pulls in jekyll-feed,
# jekyll-remote-theme, jekyll-seo-tag and webrick, so none of those are
# declared separately here.
gem "github-pages", "~> 232", group: :jekyll_plugins

# Windows-only support gems for the local `bundle exec jekyll serve` in
# jekyll.cmd. Scoped by platform so they are absent from the Linux resolve
# GitHub Pages performs and cannot conflict with its prebuilt bundle.
platforms :mingw, :x64_mingw, :mswin, :jruby do
  gem "tzinfo", "~> 1.2"
  gem "tzinfo-data"
  gem "wdm", "~> 0.1.1"
end
