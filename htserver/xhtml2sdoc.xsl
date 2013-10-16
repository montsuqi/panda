<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet version="1.0"
  xmlns:ht="http://www.w3.org/1999/xhtml" 
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:date="http://exslt.org/dates-and-times"
  extension-element-prefixes="date">

  <xsl:output method="xml" encoding="utf-8" indent="yes"/>
  <xsl:template match="/">
    <xsl:apply-templates/>
  </xsl:template>

  <xsl:template match="ht:html">
    <doc>
      <head>
        <author>anonymous</author>
      </head>
      <body>
        <xsl:apply-templates/>
      </body>
    </doc>
  </xsl:template>

  <xsl:template match="ht:body">
    <xsl:apply-templates/>
  </xsl:template>

  <xsl:template match="ht:br"/>

  <xsl:template match="ht:strong">
    <b>
      <xsl:apply-templates/>
    </b>
  </xsl:template>

  <xsl:template match="ht:em">
    <i>
      <xsl:apply-templates/>
    </i>
  </xsl:template>

  <xsl:template match="ht:u">
    <u>
      <xsl:apply-templates/>
    </u>
  </xsl:template>

  <xsl:template match="ht:strike">
    <strike>
      <xsl:apply-templates/>
    </strike>
  </xsl:template>

  <xsl:template match="ht:ul">
    <ul>
      <xsl:apply-templates/>
    </ul>
  </xsl:template>

  <xsl:template match="ht:ol">
    <ol>
      <xsl:apply-templates/>
    </ol>
  </xsl:template>

  <xsl:template match="ht:li">
    <li>
      <xsl:apply-templates/>
    </li>
  </xsl:template>

  <xsl:template match="ht:div">
    <div>
      <xsl:attribute name="align">
        <xsl:value-of select="@align"/>
      </xsl:attribute>
      <xsl:apply-templates/>
    </div>
  </xsl:template>

  <xsl:template match="ht:img">
    <img>
      <xsl:attribute name="src">
        <xsl:value-of select="@src"/>
      </xsl:attribute>
      <xsl:if test="@alt">
        <xsl:attribute name="title">
          <xsl:value-of select="@alt"/>
        </xsl:attribute>
      </xsl:if>
    </img>
  </xsl:template>

  <xsl:template match="ht:a">
    <a>
      <xsl:attribute name="href">
        <xsl:value-of select="@href"/>
      </xsl:attribute>
      <xsl:if test="@target">
        <xsl:attribute name="target">
          <xsl:value-of select="@target"/>
        </xsl:attribute>
      </xsl:if>
      <xsl:apply-templates/>
    </a>
  </xsl:template>

  <xsl:template match="ht:table">
    <table>
      <tbody>
        <xsl:apply-templates/>
      </tbody>
    </table>
  </xsl:template>

  <xsl:template match="ht:tr">
    <tr>
      <xsl:apply-templates/>
    </tr>
  </xsl:template>

  <xsl:template match="ht:td">
    <td>
      <xsl:apply-templates/>
    </td>
  </xsl:template>

  </xsl:stylesheet>
