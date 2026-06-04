# 微软NLS文件格式 | Demon's Blog
最近一直在纠缠Ruby的字符串编码问题，其中就涉及到了CP936、CP950和CP951等代码页的码表。想说与其去翻不知道靠谱与否的资料，不如直接从系统里的NLS文件中提取数据，这又牵涉到了NLS的文件格式问题。

网上能找到的NLS文件格式信息很少，[Konstantin Kazarnovsky](http://webcenter.ru/%7Ekazarn/eng/nls.htm)童鞋在2002年写的一篇是其中最详细的了。不过比对一下c\_936.nls等双字节编码发现，那篇东西错处还是不少，表格也很不知所云。于是打开WinHex猜了老半天，算是有了一点成果吧。

注：NT和非NT系统的NLS文件格式有所不同，下面的内容只适用于NT系统内的NLS文件。

以下是文件头信息，还蛮简单的：


|地址  |字节长|备注                      |
|----|---|------------------------|
|0x00|2  |文件标志，NT内的NLS应为0x000D（注一）|
|0x02|2  |代码页，如936                |
|0x04|2  |1表示单字节编码，2表示双字节编码       |
|0x06|8  |四个0x003F，疑似为转换表中的无效值（注二）|
|0x0E|12 |前导字节（注三）                |


注一：Win9X内的NLS文件中，该字段为0表示SBCS编码，为1表示DBCS编码。

注二：根据ANSI/OEM代码页以及SBCS/DBCS的不同，NLS文件中最多会有四张转换表，此处的四个DWORD很可能表示四张表中无对应字符时的值，不过貌似所有NLS文件此处都是四个0x003F就是。另外Win32中[CPINFOEX结构体](http://msdn.microsoft.com/en-us/library/dd317781%28VS.85%29.aspx)内有一个UnicodeDefaultChar成员似乎也都返回0x003F，或许就是这么来的。

注三：针对DBCS编码，此处保存第一字节可能的取值，SBCS编码该部分全为0，DBCS如CP936就是0x81和0xFE。取这12个字节的最小值并左移8位，如CP936就是0x8100，下面提到的CP2UC表DBCS部分，就是从这个值开始保存的。呃，至少CP936和CP950是这样。

接下来是Codepage To Unicode（下称CP2UC）和Unicode To Codepage（下称UC2CP）转换表。根据ANSI/OEM代码页以及SBCS/DBCS的不同，转换表一共可能有二到四张。


|地址                 |字节长             |备注                                    |
|-------------------|----------------|--------------------------------------|
|0x1A               |2               |CP2UC转换表长度（@t_len），类型和单位都是DWORD（注一）   |
|0x1C               |512             |CP2UC转换表，0x00到0xFF部分，每字符2字节           |
|0x021C             |6 / 2           |疑似分隔符，ANSI SBCS代码页此处6字节长，否则2字节长（注二）   |
|0x021E             |512             |CP2UC转换表，OEM部分，每字符2字节，ANSI SBCS代码页无此部分|
|0x041E             |4 / 2           |疑似分隔符，OEM SBCS代码页此处4字节长，否则2字节长        |
|0x0420             |(@t_len – 515)*2|CP2UC转换表，双字节部分，每字符2字节，非DBCS代码表无此部分    |
|-（注三）              |2               |疑似分隔符，非DBCS代码表无此部分                    |
|0x0222 / 0x0422 / –|65536 / 131072  |UC2CP转换表，SBCS代码页每字符1字节，否则每字符2字节       |


注一：该值其实就三种，ANSI SBCS为0x103，OEM SBCS为0x203，其他则为DBCS。

注二：间隔符共6字节。CP2UC可能有1到3张表，每两张表之间间隔2字节，末尾补齐6字节。间隔符并不总是0，是否有啥含义未知。

注三：嗯，随便注一下，0x420 + (@t\_len – 515)\*2，谁不会算呢？

上表貌似画得挺复杂，可我已经尽力了……

从文件格式来看，NLS最多只能处理二字节编码，而GB18030最长需要4个字节，难怪CP54936不可能成为系统代码页。而且NLS格式的 Unicode部分只覆盖了BMP，但较新版本的Big5-HKSCS有相当一部分却是在SIP（0x2XXXX），恐怕这也是CP951只能支持到 Big5-HKSCS:2001的原因。

再次感叹一下，Ruby 1.9居然还在跟ANSI纠缠，实在是馊到不能再馊的馊主意，真不知道那几枚大神究竟是怎么想的。

最后是代码，随便参考一下吧：

> ```
require &apos;stringio&apos;

module OtNtH;module FileFormat
  class MS_NLS
    attr_reader :codepage, :bytes_pre_char, :leadbytes
    attr_reader :oem_to_uc, :cp_to_uc, :uc_to_cp
    
    def initialize(fpath)
      s = StringIO.new(File.open(fpath, &apos;rb&apos;) { |f|f.read }, &apos;rb&apos;)
      def s.read_i2
        self.readbyte + (self.readbyte << 8)
      end

      # 文件头标志，0x0d表示NT格式的NLS
      buf = s.read_i2
      if buf == 0 then
        raise &apos;Win9x single-byte (SBCS) NLS Format!&apos;
      elsif buf == 1 then
        raise &apos;Win9x double-byte (DBCS) NLS Format!&apos;
      elsif buf != 0x0d
        raise &apos;Unknow File Format!&apos;
      end
      
      @codepage = s.read_i2
      @bytes_pre_char = s.read_i2 # 为1则是SBCS单字节编码，为2则是DBCS
      
      s.pos += 8 # 0x003F * 4，可能是CPINFOEX里边的UnicodeDefaultChar
      
      buf = s.read(12) # 前导字节，如cp936是81和fe
      @leadbytes = []
      buf.strip.each_byte { |b| @leadbytes.push(b.to_s(16)) }
      
      @t_len = s.read_i2 # 转换表长度，cp936为32771
      # 0x0103表示单字节ANSI代码页，0x0203表示单字节OEM代码页
      # 为其他值时应该都是DBCS代码页
      raise &apos;Broken NLS File?&apos; if s.size != s.pos + @t_len*2 + 0x10000*@bytes_pre_char
      
      @oem_to_uc = {}
      @cp_to_uc = {}
      @uc_to_cp = {}
      
      # 所有代码页都有的一张表
      0x100.times { |n| @cp_to_uc[sprintf(&apos;0x%.4X&apos;, n)] = sprintf(&apos;0x%.4X&apos;, s.read_i2) }
      
      while true do
        if @t_len == 0x103 then
          s.pos += 6
          break
        end
        
        s.pos += 2 # 未知内容，437为1，936和950都是0
        0x100.times { |n| @oem_to_uc[sprintf(&apos;0x%.4X&apos;, n)] = sprintf(&apos;0x%.4X&apos;, s.read_i2) }
        if @t_len == 0x203 then
          s.pos += 4
          break
        end
        
        s.pos += 2 # 未知内容，cp936是0
        b = (@leadbytes.min + &apos;00&apos;).to_i(16)
        # 256 + 1 + 256 + 1 + .... + 1
        (@t_len-515).times { |n| @cp_to_uc[sprintf(&apos;0x%.4X&apos;, b+n)] = sprintf(&apos;0x%.4X&apos;, s.read_i2) }
        
        s.pos += 2 # 未知内容，936和950为4
        break
      end
      read_unit = @bytes_pre_char == 1 ? :readbyte : :read_i2
      0x10000.times { |n| @uc_to_cp[sprintf(&apos;0x%.4X&apos;, n)] = sprintf(&apos;0x%.4X&apos;, s.send(read_unit))}
      
    end
  end
end;end

nls = OtNtH::FileFormat::MS_NLS.new(ARGV[0])
ms = {}
nls.cp_to_uc.each do |cp, uc|
  n = uc.hex
  next if uc == &apos;0x003F&apos; || cp.hex == n || n == 0
  ms[cp] = uc.to_i(16)
end

n = 0
ms.each do |cp, uc|
  n += 1 if cp.hex != nls.uc_to_cp[&apos;0x%.4X&apos; % uc].hex
end
puts n
```


_原文链接：_[_http://otnth.blogspot.com/2010/11/microsoft-nls-file-format.html_](http://otnth.blogspot.com/2010/11/microsoft-nls-file-format.html "http://otnth.blogspot.com/2010/11/microsoft-nls-file-format.html")

赞赏

![](http://demon.tw/wp-content/uploads/2022/06/3.jpg)微信赞赏![](http://demon.tw/wp-content/uploads/2022/06/9F973CCB0651C4CEEAF7635E4661E084.jpg)支付宝赞赏

随机文章:

1.  [Zen Coding for EditPlus](https://demon.tw/software/zen-coding-for-editplus.html)
2.  [C语言标准库函数rand与多线程](https://demon.tw/programming/c-rand-multi-thread.html)
3.  [PT作弊教程](https://demon.tw/copy-paste/pt-cheat-tutor.html)
4.  [BAT批处理编辑器Superbat](https://demon.tw/software/bat-editor-superbat.html)
5.  [VBS实现半角字符转全角字符](https://demon.tw/programming/vbs-half2full.html)

这篇文章发布于 2013年01月28日，星期一，23:26，归类于 [复制粘贴](https://demon.tw/category/copy-paste)。 您可以跟踪这篇文章的评论通过 [RSS 2.0](https://demon.tw/copy-paste/nls-file-format.html/feed) feed。 您可以[留下评论](#respond)，或者从您的站点[trackback](https://demon.tw/copy-paste/nls-file-format.html/trackback)。
