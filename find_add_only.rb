#!/usr/bin/env ruby
# find_multi.rb

def parse_mapping_file(filepath)
  mapping = {}
  File.foreach(filepath, encoding: 'utf-8') do |line|
    line.scan(/(\S+)\s*=>\s*(\S+)/) do |key, value|
      mapping[key.strip] = value.strip.downcase
    end
  end
  mapping
end

def normalize_hex(str)
  return str unless str =~ /^0x/i  # 非十六进制直接返回
  "0x" + str.hex.to_s(16)
end

def uni2hex(str)
  if str =~ /^u\+([0-9a-f]+)$/i
    "U+#{$1.downcase}"
  else
    raise ArgumentError, "Invalid format"
  end
end

add_file = 'add.txt'
modify_file = 'modify.txt'
cp2wc_file = 'cp2wc.txt'

puts "正在读取文件..."
add_map = parse_mapping_file(add_file)
modify_map = parse_mapping_file(modify_file)
cp2wc_map = parse_mapping_file(cp2wc_file)

puts "add.txt 条目数: #{add_map.size}"
puts "modify.txt 条目数: #{modify_map.size}"

# 收集 modify.txt 中所有的 value 作为排除集合
modify_values = modify_map.values.to_set

# 从 add.txt 中过滤出 value 不在 modify.txt 中的条目
add_only = add_map.reject { |_key, value| modify_values.include?(value) }

puts "\nadd.txt 中独有的条目（共 #{add_only.size} 条）:"
puts "-" * 60

if add_only.empty?
  puts "add.txt 中的所有 value 均在 modify.txt 中存在，无独有条目。"
else
  add_only.sort_by { |k, _v| k }.each do |key, value|
    lookup = nil
    no_lead_value = normalize_hex(value)
    if uni2hex(cp2wc_map[no_lead_value]) != key
    lookup = uni2hex(cp2wc_map[no_lead_value])
    end
    puts "#{key} => #{value} | lookup=#{lookup}"
  end
end
