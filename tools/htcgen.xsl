<?xml version="1.0" encoding="iso-2022-jp"?>
<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:date="http://exslt.org/dates-and-times"
  extension-element-prefixes="date">
  
  <!-- xsl:import href="date.xsl" / -->
  <!-- htc converter for glprotocol ver 2.0 -->

  <xsl:output method="html" encoding="iso-2022-jp"/>

  <xsl:template match="/">
    <htc coding="iso-2022-jp">
      <html>
        <head>
          <meta name="robots" content="noindex,nofollow"/>
          <style type="text/css">
td {
  font-size: 10pt;
}
</style>
		  <title>
            <xsl:value-of select="//widget/title"/>
          </title>
          <xsl:comment>
            <xsl:value-of select="date:year()"/>
            <xsl:text>/</xsl:text>
            <xsl:value-of select="date:month-in-year()"/>
            <xsl:text>/</xsl:text>
            <xsl:value-of select="date:day-in-month()"/>
          </xsl:comment>
        </head>
        <body text="#202020" bgcolor="#C0C0C0" link="#00FFFF" vlink="#00BBBB">
          <h1>
            <xsl:value-of select="//widget/title"/>
          </h1>
          <form>
            <xsl:apply-templates/>
          </form>
        </body>
      </html>
    </htc>
  </xsl:template>

  <xsl:template match="project"/>
  <xsl:template match="title"/>
  <xsl:template match="type"/>
  <xsl:template match="position"/>
  <xsl:template match="modal"/>
  <xsl:template match="allow_grow"/>
  <xsl:template match="allow_shrink"/>
  <xsl:template match="auto_shrink"/>
  <xsl:template match="justify"/>
  <xsl:template match="wrap"/>
  <xsl:template match="xalign"/>
  <xsl:template match="yalign"/>
  <xsl:template match="xpad"/>
  <xsl:template match="ypad"/>
  <xsl:template match="padding"/>
  <xsl:template match="expand"/>
  <xsl:template match="label_xalign"/>
  <xsl:template match="label_yalign"/>
  <xsl:template match="signal"/>
  <xsl:template match="xim_enabled"/>
  <xsl:template match="editable"/>

  <xsl:variable name="winName" select="//widget/name"/>

  <xsl:template match="widget">
    <xsl:variable name="class">
      <xsl:value-of select="class"/>
    </xsl:variable>
    <xsl:choose>
      <xsl:when test="class='GtkWindow'">
        <xsl:call-template name="GtkWindow"/>
      </xsl:when>
      <xsl:when test="class='GtkScrolledWindow'">
        <xsl:call-template name="GtkWindow"/>
      </xsl:when>
      <xsl:when test="class='GtkViewport'">
        <xsl:call-template name="GtkWindow"/>
      </xsl:when>
      <xsl:when test="class='GtkVBox'">
        <xsl:call-template name="GtkVBox"/>
      </xsl:when>
      <xsl:when test="class='GtkHBox'">
        <xsl:call-template name="GtkHBox"/>
      </xsl:when>
      <xsl:when test="class='GtkTable'">
        <xsl:call-template name="GtkTable"/>
      </xsl:when>
      <xsl:when test="class='GtkEntry'">
        <xsl:call-template name="GtkEntry"/>
      </xsl:when>
      <xsl:when test="class='GtkFrame'">
        <xsl:call-template name="GtkFrame"/>
      </xsl:when>
      <xsl:when test="class='GtkPandaEntry'">
        <xsl:call-template name="GtkEntry"/>
      </xsl:when>
      <xsl:when test="class='GtkNumberEntry'">
        <xsl:call-template name="GtkEntry"/>
      </xsl:when>
      <xsl:when test="class='GtkText'">
        <xsl:call-template name="GtkText"/>
      </xsl:when>
      <xsl:when test="class='GtkPandaText'">
        <xsl:call-template name="GtkText"/>
      </xsl:when>
      <xsl:when test="class='GtkLabel'">
        <xsl:call-template name="GtkLabel"/>
      </xsl:when>
      <xsl:when test="class='GtkRadioButton'">
        <xsl:call-template name="GtkRadioButton"/>
      </xsl:when>
      <xsl:when test="class='GtkCheckButton'">
        <xsl:call-template name="GtkCheckButton"/>
      </xsl:when>
      <xsl:when test="class='GtkToggleButton'">
        <xsl:call-template name="GtkToggleButton"/>
      </xsl:when>
      <xsl:when test="class='GtkButton'">
        <xsl:call-template name="GtkButton"/>
      </xsl:when>
      <xsl:when test="class='GtkList'">
        <xsl:call-template name="GtkList"/>
      </xsl:when>
      <xsl:when test="class='GtkCList'">
        <xsl:call-template name="GtkCList"/>
      </xsl:when>
      <xsl:when test="class='GtkOptionMenu'">
        <xsl:call-template name="GtkOptionMenu"/>
      </xsl:when>
      <xsl:when test="class='GtkCombo'">
        <xsl:call-template name="GtkCombo"/>
      </xsl:when>
      <xsl:when test="class='GtkPandaCombo'">
        <xsl:call-template name="GtkCombo"/>
      </xsl:when>
      <xsl:otherwise/>
    </xsl:choose>
  </xsl:template>

  <xsl:template name="GtkWindow">
    <xsl:apply-templates select="./widget"/>
  </xsl:template>

  <xsl:template name="GtkVBox">
    <table>
      <xsl:attribute name="class">
        <xsl:text>GtkVBox</xsl:text>
      </xsl:attribute>
      <xsl:for-each select="./widget">
        <tr valign="top">
          <td>
            <xsl:apply-templates select="."/>
          </td>
        </tr>
      </xsl:for-each>
    </table>
  </xsl:template>

  <xsl:template name="GtkHBox">
    <table>
      <xsl:attribute name="class">
        <xsl:text>GtkHBox</xsl:text>
      </xsl:attribute>
      <tr valign="top">
        <!-- xsl:apply-templates select="./widget"/ -->
        <xsl:for-each select="./widget">
          <td>
            <xsl:apply-templates select="."/>
          </td>
        </xsl:for-each>
      </tr>
    </table>
  </xsl:template>

  <xsl:template name="GtkFrame">
    <table>
      <xsl:choose>
        <xsl:when test="./shadow_type = 'GTK_SHADOW_NONE'"/>
        <xsl:otherwise>
          <xsl:attribute name="border">
            <xsl:value-of select="1"/>
          </xsl:attribute>
        </xsl:otherwise>
      </xsl:choose>
      <xsl:if test="./label">
        <tr>
          <td>
            <xsl:choose>
              <xsl:when test="string-length(translate(./label,'xX','')) &gt; 0">
                <xsl:value-of select="./label"/>
              </xsl:when>
              <xsl:otherwise>
                <fixed>
                  <xsl:attribute name="name">
                    <xsl:value-of select="$winName"/>
                    <xsl:text>.</xsl:text>
                    <xsl:value-of select="./name"/>
                    <xsl:text>.label</xsl:text>
                  </xsl:attribute>
                </fixed>
              </xsl:otherwise>
            </xsl:choose>
          </td>
        </tr>
      </xsl:if>
      <tr>
        <td>
          <xsl:if test="./height">
            <xsl:attribute name="height">
              <xsl:value-of select="./height"/>
            </xsl:attribute>
          </xsl:if>
          <xsl:if test="./width">
            <xsl:attribute name="width">
              <xsl:value-of select="./width"/>
            </xsl:attribute>
          </xsl:if>
          <xsl:if test="./widget">
            <xsl:apply-templates select="./widget"/>
          </xsl:if>
      </td>
    </tr>
    </table>
  </xsl:template>

  <xsl:template name="GtkTable">
    <xsl:variable name="rows" select="number(./rows)"/>
    <xsl:variable name="cols" select="number(./columns)"/>
    <table>
      <xsl:attribute name="border">
        <xsl:text>0</xsl:text>
      </xsl:attribute>
      <xsl:variable name="_rows" select="number(-1)"/>
      <xsl:call-template name="Rows">
        <xsl:with-param name="_rows" select="$_rows + 1"/>
        <xsl:with-param name="_rmax" select="$rows"/>
        <xsl:with-param name="_cmax" select="$cols"/>
      </xsl:call-template>
    </table>
  </xsl:template>

  <xsl:template name="Rows">
    <xsl:param name="_rows"/>
    <xsl:param name="_rmax"/>
    <xsl:param name="_cmax"/>
    <xsl:if test="$_rows &lt; $_rmax">
      <tr>
        <xsl:variable name="_cols" select="number(-1)"/>
        <xsl:call-template name="Cols">
          <xsl:with-param name="_rows" select="$_rows"/>
          <xsl:with-param name="_cols" select="$_cols + 1"/>
          <xsl:with-param name="_cmax" select="$_cmax"/>
        </xsl:call-template>
      </tr>
      <xsl:call-template name="Rows">
        <xsl:with-param name="_rows" select="$_rows + 1"/>
        <xsl:with-param name="_rmax" select="$_rmax"/>
        <xsl:with-param name="_cmax" select="$_cmax"/>
      </xsl:call-template>
    </xsl:if>
  </xsl:template>

  <xsl:template name="Cols">
    <xsl:param name="_rows"/>
    <xsl:param name="_cols"/>
    <xsl:param name="_cmax"/>
    <xsl:if test="$_cols &lt; $_cmax">
      <td>
        <xsl:for-each select="./widget/child">
          <xsl:if test="number(top_attach) = $_rows and number(left_attach) = $_cols">
            <xsl:apply-templates select=".."/>
          </xsl:if>
        </xsl:for-each>
      </td>
      <xsl:call-template name="Cols">
        <xsl:with-param name="_rows" select="$_rows"/>
        <xsl:with-param name="_cols" select="$_cols + 1"/>
        <xsl:with-param name="_cmax" select="$_cmax"/>
      </xsl:call-template>
    </xsl:if>
  </xsl:template>

  <xsl:template name="GtkEntry">
    <entry>
      <xsl:attribute name="name">
        <xsl:value-of select="$winName"/>
        <xsl:text>.</xsl:text>
        <xsl:value-of select="./name"/>
        <xsl:text>.value</xsl:text>
      </xsl:attribute>
      <xsl:choose>
        <xsl:when test="./width">
          <xsl:attribute name="size">
            <xsl:value-of select="floor(number(./width) div 8)"/>
          </xsl:attribute>
        </xsl:when>
        <xsl:when test="./text_max_length">
          <xsl:attribute name="size">
            <xsl:choose>
              <xsl:when test="number(./text_max_length) &gt; 50">
                <xsl:value-of select="floor(number(./text_max_length) div 10)"/>
              </xsl:when>
              <xsl:otherwise>
                <xsl:value-of select="number(./text_max_length)"/>
              </xsl:otherwise>
            </xsl:choose>
          </xsl:attribute>
        </xsl:when>
      </xsl:choose>
      <xsl:if test="./text_max_length">
        <xsl:attribute name="maxlength">
          <xsl:value-of select="number(./text_max_length)"/>
        </xsl:attribute>
      </xsl:if>
    </entry>
  </xsl:template>
  
  <xsl:template name="GtkText">
    <text>
      <xsl:attribute name="name">
        <xsl:value-of select="$winName"/>
        <xsl:text>.</xsl:text>
        <xsl:value-of select="./name"/>
        <xsl:text>.value</xsl:text>
      </xsl:attribute>
    </text>
  </xsl:template>

  <xsl:template name="GtkLabel">
    <div>
      <xsl:choose>
        <xsl:when test="string-length(translate(./label,'xX','')) &gt; 0">
          <xsl:value-of select="./label"/>
        </xsl:when>
        <xsl:otherwise>
          <fixed>
            <xsl:attribute name="name">
              <xsl:value-of select="$winName"/>
              <xsl:text>.</xsl:text>
              <xsl:value-of select="./name"/>
              <xsl:text>.value</xsl:text>
            </xsl:attribute>
          </fixed>
        </xsl:otherwise>
      </xsl:choose>
    </div>
  </xsl:template>

  <xsl:template name="GtkCombo">
    <combo>
      <xsl:attribute name="count">
        <xsl:value-of select="$winName"/>
        <xsl:text>.</xsl:text>
        <xsl:value-of select="./name"/>
        <xsl:text>.count</xsl:text>
      </xsl:attribute>
      <xsl:attribute name="name">
        <xsl:value-of select="$winName"/>
        <xsl:text>.</xsl:text>
        <xsl:value-of select="./widget[class = 'GtkEntry']/name"/>
        <xsl:text>.value</xsl:text>
      </xsl:attribute>
      <xsl:attribute name="item">
        <xsl:value-of select="$winName"/>
        <xsl:text>.</xsl:text>
        <xsl:value-of select="./name"/>
        <xsl:text>.item</xsl:text>
      </xsl:attribute>
    </combo>
  </xsl:template>

  <xsl:template name="GtkRadioButton">
    <radiobutton>
      <xsl:attribute name="group">
        <xsl:value-of select="./group"/>
      </xsl:attribute>
      <xsl:attribute name="name">
        <xsl:value-of select="$winName"/>
        <xsl:text>.</xsl:text>
        <xsl:value-of select="./name"/>
        <xsl:text>.value</xsl:text>
      </xsl:attribute>
      <xsl:if test="./label">
        <xsl:attribute name="label">
          <xsl:value-of select="./label"/>
        </xsl:attribute>
      </xsl:if>
    </radiobutton>
  </xsl:template>

  <xsl:template name="GtkCheckButton">
    <checkbutton>
      <xsl:attribute name="name">
        <xsl:value-of select="$winName"/>
        <xsl:text>.</xsl:text>
        <xsl:value-of select="./name"/>
        <xsl:text>.value</xsl:text>
      </xsl:attribute>
      <xsl:if test="./label">
        <xsl:attribute name="label">
          <xsl:value-of select="./label"/>
        </xsl:attribute>
      </xsl:if>
    </checkbutton>
  </xsl:template>

  <xsl:template name="GtkToggleButton">
    <togglebutton>
      <xsl:attribute name="name">
        <xsl:value-of select="$winName"/>
        <xsl:text>.</xsl:text>
        <xsl:value-of select="./name"/>
        <xsl:text>.value</xsl:text>
      </xsl:attribute>
      <xsl:if test="./label">
        <xsl:attribute name="label">
          <xsl:value-of select="./label"/>
        </xsl:attribute>
      </xsl:if>
    </togglebutton>
  </xsl:template>

  <xsl:template name="GtkButton">
    <button>
      <xsl:attribute name="event">
        <xsl:value-of select="./signal/data"/>
        <xsl:text>:</xsl:text>
        <xsl:value-of select="./name"/>
      </xsl:attribute>
      <xsl:attribute name="face">
        <xsl:choose>
          <xsl:when test="./label">
            <xsl:value-of select="./label"/>
          </xsl:when>
          <xsl:when test="./stock_button">
            <xsl:choose>
              <xsl:when test="./stock_button = 'GNOME_STOCK_BUTTON_OK'">
                <xsl:text>OK</xsl:text>
              </xsl:when>
              <xsl:when test="./stock_button = 'GNOME_STOCK_BUTTON_CANCEL'">
                <xsl:text>CANCEL</xsl:text>
              </xsl:when>
              <xsl:otherwise/>
            </xsl:choose>
          </xsl:when>
        </xsl:choose>
      </xsl:attribute>
    </button>
  </xsl:template>

  <xsl:template name="GtkOptionMenu">
    <optionmenu>
      <xsl:attribute name="count">
        <xsl:value-of select="$winName"/>
        <xsl:text>.</xsl:text>
        <xsl:value-of select="./name"/>
        <xsl:text>.count</xsl:text>
      </xsl:attribute>
      <xsl:attribute name="select">
        <xsl:value-of select="$winName"/>
        <xsl:text>.</xsl:text>
        <xsl:value-of select="./name"/>
        <xsl:text>.select</xsl:text>
      </xsl:attribute>
      <xsl:attribute name="item">
        <xsl:value-of select="$winName"/>
        <xsl:text>.</xsl:text>
        <xsl:value-of select="./name"/>
        <xsl:text>.item</xsl:text>
      </xsl:attribute>
    </optionmenu>
  </xsl:template>

  <xsl:template name="MakeWidth">
    <xsl:param name="_width"/>
    <xsl:text>&#160;</xsl:text>
    <xsl:if test="$_width &gt; 0">
      <xsl:call-template name="MakeWidth">
        <xsl:with-param name="_width" select="$_width - 1"/>
      </xsl:call-template>
    </xsl:if>
  </xsl:template>

  <xsl:template name="GtkList">
    <select>
      <xsl:attribute name="name">
        <xsl:value-of select="$winName"/>
        <xsl:text>.</xsl:text>
        <xsl:value-of select="./name"/>
        <xsl:text>.select</xsl:text>
      </xsl:attribute>
      <xsl:attribute name="size">
        <xsl:choose>
          <xsl:when test="../../widget/height">
            <xsl:value-of select="floor(number(../../widget/height) div 12)"/>
          </xsl:when>
          <xsl:when test="../../widget/class = 'GtkViewport'">
            <xsl:value-of select="4"/>
          </xsl:when>
        </xsl:choose>
      </xsl:attribute>
      <xsl:choose>
        <xsl:when test="./selection_mode = 'GTK_SELECTION_SINGLE'"/>
        <xsl:otherwise>
          <xsl:attribute name="multiple"/>
        </xsl:otherwise>
      </xsl:choose>
      <option>
        <xsl:attribute name="value">
          <xsl:text>-1</xsl:text>
        </xsl:attribute>
        <xsl:choose>
          <xsl:when test="./column_widths">
            <xsl:call-template name="MakeWidth">
              <xsl:with-param name="_width" select="floor(number(./column_widths) div 2.5)"/>
            </xsl:call-template>
          </xsl:when>
          <xsl:otherwise>
            <xsl:call-template name="MakeWidth">
              <xsl:with-param name="_width" select="20"/>
            </xsl:call-template>
          </xsl:otherwise>
        </xsl:choose>
      </option>
      <count>
        <xsl:attribute name="var">
          <xsl:text>i</xsl:text>
        </xsl:attribute>
        <xsl:attribute name="from">
          <xsl:text>0</xsl:text>
        </xsl:attribute>
        <xsl:attribute name="to">
          <xsl:value-of select="$winName"/>
          <xsl:text>.</xsl:text>
          <xsl:value-of select="./name"/>
          <xsl:text>.count</xsl:text>
        </xsl:attribute>
        <option>
          <xsl:attribute name="value">
            <xsl:text>$$i</xsl:text>
          </xsl:attribute>
          <xsl:attribute name="select">
            <xsl:value-of select="$winName"/>
            <xsl:text>.</xsl:text>
            <xsl:value-of select="./name"/>
            <xsl:text>.select[#i]</xsl:text>
          </xsl:attribute>
          <fixed>
            <xsl:attribute name="name">
              <xsl:value-of select="$winName"/>
              <xsl:text>.</xsl:text>
              <xsl:value-of select="./name"/>
              <xsl:text>.item[#i]</xsl:text>
            </xsl:attribute>
          </fixed>
        </option>
      </count>
    </select>
  </xsl:template>

  <xsl:template name="GtkCList">
    <select>
      <xsl:attribute name="name">
        <xsl:value-of select="$winName"/>
        <xsl:text>.</xsl:text>
        <xsl:value-of select="./name"/>
        <xsl:text>.select</xsl:text>
      </xsl:attribute>
      <xsl:attribute name="size">
        <xsl:choose>
          <xsl:when test="../../widget/height">
            <xsl:value-of select="floor(number(../../widget/height) div 12)"/>
          </xsl:when>
          <xsl:when test="../../widget/class = 'GtkViewport'">
            <xsl:value-of select="4"/>
          </xsl:when>
        </xsl:choose>
      </xsl:attribute>
      <xsl:choose>
        <xsl:when test="./selection_mode = 'GTK_SELECTION_SINGLE'"/>
        <xsl:otherwise>
          <xsl:attribute name="multiple"/>
        </xsl:otherwise>
      </xsl:choose>
      <option>
        <xsl:attribute name="value">
          <xsl:text>-1</xsl:text>
        </xsl:attribute>
        <xsl:choose>
          <xsl:when test="./column_widths">
            <xsl:call-template name="MakeWidth">
              <xsl:with-param name="_width" select="floor(number(./column_widths) div 2.5)"/>
            </xsl:call-template>
          </xsl:when>
          <xsl:otherwise>
            <xsl:call-template name="MakeWidth">
              <xsl:with-param name="_width" select="20"/>
            </xsl:call-template>
          </xsl:otherwise>
        </xsl:choose>
      </option>
      <count>
        <xsl:attribute name="var">
          <xsl:text>i</xsl:text>
        </xsl:attribute>
        <xsl:attribute name="from">
          <xsl:text>0</xsl:text>
        </xsl:attribute>
        <xsl:attribute name="to">
          <xsl:value-of select="$winName"/>
          <xsl:text>.</xsl:text>
          <xsl:value-of select="./name"/>
          <xsl:text>.count</xsl:text>
        </xsl:attribute>
        <option>
          <xsl:attribute name="value">
            <xsl:text>$$i</xsl:text>
          </xsl:attribute>
          <xsl:attribute name="select">
            <xsl:value-of select="$winName"/>
            <xsl:text>.</xsl:text>
            <xsl:value-of select="./name"/>
            <xsl:text>.select[#i]</xsl:text>
          </xsl:attribute>
          <fixed>
            <xsl:attribute name="name">
              <xsl:value-of select="$winName"/>
              <xsl:text>.</xsl:text>
              <xsl:value-of select="./name"/>
              <xsl:text>.item[#i].value1</xsl:text>
            </xsl:attribute>
          </fixed>
        </option>
      </count>
    </select>
  </xsl:template>


</xsl:stylesheet>
