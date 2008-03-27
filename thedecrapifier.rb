#!/usr/bin/env ruby
require 'rubygems'
require 'optparse'


options = {
  :directory => ".",
  :pattern => /\.html/,
  :backup => true,
  :tidy => false,
  :tidy_path => "/usr/lib/libtidy.dylib",
  :begin_pattern => /\<\!--\s*BEGIN EDITORIAL CONTENT\s*--\>/,
  :end_pattern => /\<\!--\s*END EDITORIAL CONTENT\s*--\>/,
  :header_tpl => "header.tpl",
  :footer_tpl => "footer.tpl",
  :xhtml => false
}
OptionParser.new do |opts|
  opts.on("-d", "--directory DIR", "Directory where to run this") do |d|
    options[:directory] = d
  end
  
  opts.on("-p", "--patern PAT", "Regex pattern to match files against") do |p|
    options[:pattern] = Regexp.new(p)
  end
  
  opts.on("-b", "--backup YES|NO", "Whether or not to backup the existing file before decrapifying it") do |b|
    options[:backup] = (b=="YES")
  end
  
  opts.on("-t", "--tidy YES|NO", "Whether or not to run Tidy") do |t|
    options[:tidy] = (t=="YES")
  end
  
  opts.on("-a", "--tidy-path PATH", "Path to your system's Tidy dylib, ex: /usr/lib/libtidy.dylib") do |path|
    options[:tidy_path] = path
  end
  
  opts.on("-x", "--xhtml YES|NO", "Output tidy content as XHTML") do |xhtml|
    options[:xhtml] = true
  end
  
  opts.on("-s", "--start PAT", "Regex pattern that marks start of content to extract") do |start|
    options[:begin_pattern] = Regexp.new(start)
  end
  
  opts.on("-f", "--finish PAT", "Regex pattern that marks end of content to extract") do |finish|
    options[:end_pattern] = Regexp.new(finish)
  end
  
  opts.on("-h", "--header TPL", "File to use as the new header content") do |header|
    options[:header_tpl] = header
  end
  
  opts.on("-o", "--footer TPL", "File to use as the new footer content") do |footer|
    options[:footer_tpl] = footer
  end
  

  

end.parse!(ARGV)

if options[:tidy]
  require 'tidy' 
  Tidy.path = options[:tidy_path]
end

hcontent = File.read(options[:header_tpl])
fcontent = File.read(options[:footer_tpl])
d = Dir.entries(options[:directory])
d.each do |fn|
  if fn =~ options[:pattern]
    fpath = File.join(options[:directory], fn)
    puts "Decrapifying #{fpath}"
    content = File.read(fpath)
    if options[:backup]
      bak = File.new(fpath + ".bak", "w+")
      bak.write(content)
      bak.close
    end
    p1 = content =~ options[:begin_pattern]
    p2 = content =~ options[:end_pattern]
    if p1 && p2
      ex = content[p1..(p2 + Regexp.last_match[0].size)]
      newcontent = hcontent + ex + fcontent
      if options[:tidy]
        newcontent = Tidy.open(:show_warnings => true,
                                :tidy_mark => false) do |tidy|
          tidy.options.output_xhtml = true if options[:xhtml]
          newcontent = tidy.clean(newcontent)
          puts tidy.errors
          puts tidy.diagnostics
          newcontent
        end
      end
      f = File.new(fpath, "w+")
      f.write(newcontent)
      f.close
    else
      puts " - #{fpath} did not match suggested patterns, cannot decrapify"
    end
      
  end
end