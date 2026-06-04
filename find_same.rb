#!/usr/bin/env ruby
# find_common_values.rb

def parse_mapping_file(filepath)
  mapping = {}
  File.foreach(filepath, encoding: 'utf-8') do |line|
    # 使用正则匹配所有 "Key => Value" 格式的条目
    # 支持一行中存在多个条目的情况（如 add.txt 中的部分行）
    line.scan(/(\S+)\s*=>\s*(\S+)/) do |key, value|
      mapping[key.strip] = value.strip.downcase
    end
  end
  mapping
end

# 文件路径配置
add_file = 'add.txt'
modify_file = 'modify.txt'

puts "正在读取文件..."
add_map = parse_mapping_file(add_file)
modify_map = parse_mapping_file(modify_file)

puts "add.txt 条目数: #{add_map.size}"
puts "modify.txt 条目数: #{modify_map.size}"

# 找出具有相同 value 的条目
# 构建 value -> [keys] 的索引以便快速查找
add_value_index = add_map.each_with_object(Hash.new([])) { |(k, v), h| h[v] += [k] }
modify_value_index = modify_map.each_with_object(Hash.new([])) { |(k, v), h| h[v] += [k] }

# 获取两个文件中共同出现的 values
common_values = add_value_index.keys & modify_value_index.keys

puts "\n找到 #{common_values.size} 个相同的 value:"
puts "-" * 60

if common_values.empty?
  puts "未发现具有相同 value 的条目。"
else
  common_values.sort.each do |val|
    add_keys = add_value_index[val]
    mod_keys = modify_value_index[val]
    
    puts "Value: #{val}"
    puts "  add.txt    keys: #{add_keys.join(', ')}"
    puts "  modify.txt keys: #{mod_keys.join(', ')}"
    puts
  end
end
