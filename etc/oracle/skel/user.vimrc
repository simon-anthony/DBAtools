" $Header$
if version >= 700
set helplang=en
set fileencodings=utf-8,latin1
if &cp | set nocp | endif
let s:cpo_save=&cpo
set cpo&vim
let &cpo=s:cpo_save
unlet s:cpo_save
colorscheme doritos
endif
set backspace=indent,eol,start
set cscopeprg=/usr/bin/cscope
set cscopetag
set cscopeverbose
set formatoptions=tcql
set history=50
set hlsearch
set ruler
set textwidth=78
set viminfo='20,\"50
set autoindent
set showmatch
set wrapmargin=0
set report=1
set tabstop=4
set shiftwidth=4
" 256 color support
" for old versions of PuTTY < 0.60
if &term =~ "xterm"
	let &t_Co=256
	set t_ti=7[r[?47h
	set t_te=[?47l8
	if has("terminfo")
		let &t_Sf="[3%p1%dm"
		let &t_Sb="[4%p1%dm"
	else
		let &t_Sf="[3%dm"
		let &t_Sb="[4%dm"
	endif
	syntax on
endif
map  <F3>   :let @/ = ""
map! <F3> :let @/ = ""
"
" large keypad
" Home
map [1~ H
" End
map [4~ G
" PgUp
map [5~ 
" PgDown
map [6~ 
"
" small keypad mapped to its logically named equivalents
" Home
map OH H
" End
map [OF G
" PgUp
map [5~ 
" PgDn
map [6~ 
