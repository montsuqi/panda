<?xml version="1.0" encoding="iso-2022-jp"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  <!-- htc converter for glprotocol ver 2.0 -->

  <xsl:output method="html" encoding="iso-2022-jp"/>
  <!-- xsl:variable name="genType" select="'htc'"/ -->
  <xsl:variable name="genType" select="'html'"/>

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
      <xsl:when test="class='GtkButton'">
        <xsl:call-template name="GtkButton"/>
      </xsl:when>
      <xsl:when test="class='GtkList'">
        <xsl:call-template name="GtkList"/>
      </xsl:when>
      <xsl:when test="class='GtkCList'">
        <xsl:call-template name="GtkCList"/>
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
        <tr>
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
      <tr>
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
                    <xsl:value-of select="./name"/>
                    <xsl:text>.value</xsl:text>
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
    <xsl:choose>
      <xsl:when test="$genType = 'html'">
        <input>
          <xsl:attribute name="type">
            <xsl:text>text</xsl:text>
          </xsl:attribute>
          <xsl:attribute name="size">
            <!-- xsl:value-of select="number(./width)"/ -->
            <xsl:value-of select="4"/>
          </xsl:attribute>
          <xsl:attribute name="name">
            <xsl:value-of select="./name"/>
          </xsl:attribute>
        </input>
      </xsl:when>
      <xsl:when test="$genType = 'htc'">
        <entry>
          <xsl:attribute name="name">
            <xsl:value-of select="./name"/>
          </xsl:attribute>
          <xsl:if test="./width">
            <xsl:attribute name="size">
              <xsl:value-of select="number(./width)"/>
            </xsl:attribute>
          </xsl:if>
          <xsl:if test="./text_max_length">
            <xsl:attribute name="maxlength">
              <xsl:value-of select="number(./text_max_length)"/>
            </xsl:attribute>
          </xsl:if>
        </entry>
      </xsl:when>
    </xsl:choose>
  </xsl:template>

  <xsl:template name="GtkText">
    <xsl:choose>
      <xsl:when test="$genType = 'html'">
        <textarea>
          <xsl:attribute name="name">
            <xsl:value-of select="./name"/>
          </xsl:attribute>
        </textarea>
      </xsl:when>
      <xsl:when test="$genType = 'htc'">
        <text>
          <xsl:attribute name="name">
            <xsl:value-of select="./name"/>
          </xsl:attribute>
        </text>
      </xsl:when>
    </xsl:choose>
  </xsl:template>

  <xsl:template name="GtkLabel">
    <div>
      <xsl:choose>
        <xsl:when test="string-length(translate(./label,'xX','')) &gt; 0">
          <xsl:value-of select="./label"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:choose>
            <xsl:when test="$genType = 'htc'">
              <fixed>
                <xsl:attribute name="name">
                  <xsl:value-of select="./name"/>
                  <xsl:text>.value</xsl:text>
                </xsl:attribute>
              </fixed>
            </xsl:when>
          </xsl:choose>
        </xsl:otherwise>
      </xsl:choose>
    </div>
  </xsl:template>

  <xsl:template name="GtkRadioButton">
    <xsl:choose>
      <xsl:when test="$genType = 'html'">
        <input>
          <xsl:attribute name="type">
            <xsl:text>radio</xsl:text>
          </xsl:attribute>
          <xsl:attribute name="name">
            <xsl:value-of select="./name"/>
            <xsl:text>.value</xsl:text>
          </xsl:attribute>
        </input>
        <xsl:if test="./label">
          <xsl:value-of select="./label"/>
        </xsl:if>
      </xsl:when>
      <xsl:when test="$genType = 'htc'">
        <radiobutton>
          <xsl:attribute name="group">
            <xsl:value-of select="./group"/>
          </xsl:attribute>
          <xsl:attribute name="name">
            <xsl:value-of select="./name"/>
            <xsl:text>.value</xsl:text>
          </xsl:attribute>
          <xsl:if test="./label">
            <xsl:attribute name="label">
              <xsl:value-of select="./label"/>
            </xsl:attribute>
          </xsl:if>
        </radiobutton>
      </xsl:when>
    </xsl:choose>
  </xsl:template>

  <xsl:template name="GtkCheckButton">
    <xsl:choose>
      <xsl:when test="$genType = 'html'">
        <input>
          <xsl:attribute name="type">
            <xsl:text>checkbox</xsl:text>
          </xsl:attribute>
          <xsl:attribute name="name">
            <xsl:value-of select="./name"/>
            <xsl:text>.value</xsl:text>
          </xsl:attribute>
        </input>
        <xsl:if test="./label">
          <xsl:value-of select="./label"/>
        </xsl:if>
      </xsl:when>
      <xsl:when test="$genType = 'htc'">
        <checkbutton>
          <xsl:attribute name="name">
            <xsl:value-of select="./name"/>
            <xsl:text>.value</xsl:text>
          </xsl:attribute>
          <xsl:if test="./label">
            <xsl:attribute name="label">
              <xsl:value-of select="./label"/>
            </xsl:attribute>
          </xsl:if>
        </checkbutton>
      </xsl:when>
    </xsl:choose>
  </xsl:template>

  <xsl:template name="GtkButton">
    <xsl:choose>
      <xsl:when test="$genType = 'html'">
        <input>
          <xsl:attribute name="type">
            <xsl:text>submit</xsl:text>
          </xsl:attribute>
          <xsl:attribute name="name">
            <xsl:text>_event</xsl:text>
          </xsl:attribute>
          <xsl:attribute name="value">
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
        </input>
      </xsl:when>
      <xsl:when test="$genType = 'htc'">
        <button>
          <xsl:attribute name="event">
            <xsl:value-of select="./data"/>
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
      </xsl:when>
    </xsl:choose>
  </xsl:template>

  <xsl:template name="GtkList">
    <xsl:choose>
      <xsl:when test="$genType = 'html'">
        <select>
          <xsl:attribute name="value">
            <xsl:value-of select="./name"/>
          </xsl:attribute>
          <xsl:if test="./selection_mode = 'GTK_SELECTION_MULTIPLE'">
            <xsl:attribute name="multiple"/>
          </xsl:if>
          <option>
            <xsl:text>dummy</xsl:text>
          </option>
        </select>
      </xsl:when>
      <xsl:when test="$genType = 'htc'">
        <list>
          <xsl:attribute name="name">
            <xsl:value-of select="./name"/>
            <xsl:text>.item</xsl:text>
          </xsl:attribute>
          <xsl:attribute name="value">
            <xsl:value-of select="./name"/>
            <xsl:text>.value</xsl:text>
          </xsl:attribute>
          <xsl:attribute name="count">
            <xsl:value-of select="./name"/>
            <xsl:text>.count</xsl:text>
          </xsl:attribute>
        </list>
      </xsl:when>
    </xsl:choose>
  </xsl:template>

  <xsl:template name="GtkCList">
    <xsl:choose>
      <xsl:when test="$genType = 'html'">
        <select>
          <xsl:attribute name="value">
            <xsl:value-of select="./name"/>
          </xsl:attribute>
          <xsl:if test="./selection_mode = 'GTK_SELECTION_MULTIPLE'">
            <xsl:attribute name="multiple"/>
          </xsl:if>
          <option>
            <xsl:text>dummy</xsl:text>
          </option>
        </select>
      </xsl:when>
      <xsl:when test="$genType = 'htc'">
        <list>
          <xsl:attribute name="name">
            <xsl:value-of select="./name"/>
            <xsl:text>.item</xsl:text>
          </xsl:attribute>
          <xsl:attribute name="value">
            <xsl:value-of select="./name"/>
            <xsl:text>.value</xsl:text>
          </xsl:attribute>
          <xsl:attribute name="count">
            <xsl:value-of select="./name"/>
            <xsl:text>.count</xsl:text>
          </xsl:attribute>
        </list>
      </xsl:when>
    </xsl:choose>
  </xsl:template>


</xsl:stylesheet>
